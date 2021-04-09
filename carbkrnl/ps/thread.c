


#include <carbsup.h>
#include "psp.h"
#include "../hal/halp.h"
#include "../ke/ki.h"
#include "../ob/obp.h"
#include "../mm/mi.h"
#include "../rtl/rtlp.h"

PKTHREAD
PsGetCurrentThread(

)
{
    PKTHREAD Thread;

    Thread = KeQueryCurrentProcessor( )->ThreadQueue;
    return Thread;
}

PKPROCESS
PsGetCurrentProcess(

)
{
    PKPROCESS Process;

    Process = KeQueryCurrentProcessor( )->ThreadQueue->Process;
    return Process;
}

ULONG64
PsGetThreadId(
    _In_ PKTHREAD Thread
)
{
    return Thread->ThreadId;
}

KPROCESSOR_MODE
PsGetPreviousMode(
    _In_ PKTHREAD Thread
)
{
    return Thread->PreviousMode;
}

PKPROCESS
PsGetThreadProcess(
    _In_ PKTHREAD Thread
)
{
    return Thread->Process;
}

PVOID
PspCreateStack(
    _In_ HANDLE  ProcessHandle,
    _In_ ULONG64 Length
)
{
    PVOID Base;
    PPS_SYSTEM_STACK Stack;
    KIRQL PreviousIrql;
    PKPROCESS Process;

    //
    // If ProcessHandle == 0, then you are creating a system stack, 
    // it will be linked into PsInitialSystemProcess and allocated as 
    // supervisor memory.
    //

    if ( ProcessHandle != 0 ) {

        Base = NULL;
        ZwAllocateVirtualMemory( ProcessHandle,
                                 &Base,
                                 Length,
                                 PAGE_READ | PAGE_WRITE );

        ObReferenceObjectByHandle( &Process,
                                   ProcessHandle,
                                   0,
                                   KernelMode,
                                   PsProcessObject );

        Stack = MmAllocatePoolWithTag( NonPagedPool,
                                       sizeof( PS_SYSTEM_STACK ),
                                       PS_TAG );
    }
    else {

        Base = MmAllocatePoolWithTag( NonPagedPool,
                                      Length,
                                      PS_TAG );

        Stack = MmAllocatePoolWithTag( NonPagedPool,
                                       sizeof( PS_SYSTEM_STACK ),
                                       PS_TAG );

        Process = PsInitialSystemProcess;
    }

    Stack->Address = ( ULONG64 )Base;
    Stack->Length = Length;

    KeAcquireSpinLock( &Process->StackLock, &PreviousIrql );

    Process->StackCharge += Length;

    if ( Process->StackCount == 0 ) {

        KeInitializeHeadList( &Stack->StackLinks );
        Process->StackLinks = &Stack->StackLinks;
    }
    else {

        KeInsertTailList( Process->StackLinks, &Stack->StackLinks );
    }
    Process->StackCount++;

    KeReleaseSpinLock( &Process->StackLock, PreviousIrql );

    if ( Process != PsInitialSystemProcess ) {

        ObDereferenceObject( Process );
    }

    return ( PVOID )Stack->Address;
}

VOID
PspDestroyStack(
    _In_ HANDLE  ProcessHandle,
    _In_ PVOID   Base,
    _In_ ULONG64 Length,
    _In_ BOOLEAN User
)
{
    if ( User ) {

        ZwFreeVirtualMemory( ProcessHandle, Base, Length );
    }
    else {

        MmFreePoolWithTag( Base, PS_TAG );
    }
}

PPS_SYSTEM_STACK
PspQueryStack(
    _In_ PKPROCESS Process,
    _In_ PVOID     Pointer
)
{
    KIRQL PreviousIrql;
    PLIST_ENTRY Flink;
    PPS_SYSTEM_STACK Stack;
    ULONG64 Value;

    Value = ( ULONG64 )Pointer;

    KeAcquireSpinLock( &Process->StackLock, &PreviousIrql );

    if ( Process->StackCount > 0 ) {

        Flink = Process->StackLinks;
        do {
            Stack = CONTAINING_RECORD( Flink, PS_SYSTEM_STACK, StackLinks );
            Flink = Flink->Flink;

            if ( Value >= Stack->Address &&
                 Value < Stack->Address + Stack->Length ) {

                KeReleaseSpinLock( &Process->StackLock, PreviousIrql );
                return Stack;
            }

        } while ( Flink != Process->StackLinks );
    }

    KeReleaseSpinLock( &Process->StackLock, PreviousIrql );
    return NULL;
}


ULONG64
PspGetThreadProcessor(

)
{
    ULONG64 ThreadProcessor = 0;
    ULONG64 LowestWork = ( ULONG64 )( -1 );

    for ( ULONG64 i = 0; i < KeQueryProcessorCount( ); i++ ) {

        PKPCB Processor;
        Processor = KeQueryProcessorByNumber( i );

        if ( LowestWork > Processor->ThreadQueueLength ) {
            LowestWork = Processor->ThreadQueueLength;
            ThreadProcessor = i;
        }
    }

    return ThreadProcessor;
}

NTSTATUS
ZwCreateThread(
    _Out_     PHANDLE            ThreadHandle,
    _In_      HANDLE             ProcessHandle,
    _In_      ACCESS_MASK        DesiredAccess,
    _In_      PKSTART_ROUTINE    ThreadStart,
    _In_      PVOID              ThreadContext,
    _In_      ULONG32            Flags,
    _In_      POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_  ULONG64            StackLength,
    _Out_opt_ PULONG64           ThreadId
)
{
    NTSTATUS ntStatus;
    PKTHREAD Thread;
    PKPROCESS Process;
    PKPCB Processor;
    KIRQL PreviousIrql;

    if ( ( Flags & THREAD_SYSTEM ) == 0 ) {
        ntStatus = ObReferenceObjectByHandle( &Process,
                                              ProcessHandle,
                                              PROCESS_CREATE_THREAD,
                                              KernelMode,
                                              PsProcessObject );
        if ( !NT_SUCCESS( ntStatus ) ) {

            return ntStatus;
        }
    }
    else {
        Process = PsInitialSystemProcess;
        ProcessHandle = 0;
        ObReferenceObject( Process );

        // fix.
    }

    ntStatus = ObCreateObject( &Thread,
                               PsThreadObject,
                               ObjectAttributes,
                               sizeof( KTHREAD ) );
    if ( !NT_SUCCESS( ntStatus ) ) {

        ObDereferenceObject( Process );
        return ntStatus;
    }

    ntStatus = ObOpenObjectFromPointer( ThreadHandle,
                                        Thread,
                                        DesiredAccess,
                                        ObjectAttributes->Attributes,
                                        KernelMode );
    if ( !NT_SUCCESS( ntStatus ) ) {

        ObDereferenceObject( Process );
        ObDereferenceObject( Thread );
        return ntStatus;
    }

    Thread->ThreadLock = 1;
    Thread->Header.Type = DPC_OBJECT_THREAD;

    Thread->StackLength = StackLength == 0 ? 0x8000 : StackLength;
    if ( ( Flags & THREAD_SYSTEM ) == 0 ) {

        //
        // Kernel stack is only necessary for threads
        // which will execute SYSCALL, only user mode
        // threads will.
        //
        // uh, because of the new stack system, these stacks
        // are linked to the PsInitialSystemProcess, remember
        // to free them.
        //

        Thread->KernelStackLength = 0x8000;
        Thread->KernelStackBase = ( ULONG64 )PspCreateStack( 0, Thread->KernelStackLength );
        Thread->StackBase = ( ULONG64 )PspCreateStack( ProcessHandle, Thread->StackLength );
    }
    else {

        Thread->StackBase = ( ULONG64 )PspCreateStack( ProcessHandle, Thread->StackLength );
    }

    Thread->Process = Process;

    Thread->ThreadId = KeGenerateUniqueId( );
    Thread->ProcessorNumber = PspGetThreadProcessor( );
    Thread->ExitCode = ( ULONG64 )-1;

    ObReferenceObject( Thread );

    Thread->TrapFrame.Cr3 = Process->DirectoryTableBase;
    Thread->TrapFrame.Rsp = Thread->StackBase + Thread->StackLength - 0x28;
    if ( ( Flags & THREAD_SYSTEM ) == THREAD_SYSTEM ) {

        *( ULONG64* )Thread->TrapFrame.Rsp = ( ULONG64 )PspSystemThreadReturn;
    }

    Thread->TrapFrame.EFlags = 0x202;
    Thread->TrapFrame.Rip = ( ULONG64 )ThreadStart;
    Thread->TrapFrame.Rcx = ( ULONG64 )ThreadContext;
    Thread->TrapFrame.SegCs = ( ( Flags & THREAD_SYSTEM ) == THREAD_SYSTEM ) ? GDT_KERNEL_CODE64 : GDT_USER_CODE64 | 3;
    Thread->TrapFrame.SegSs = ( ( Flags & THREAD_SYSTEM ) == THREAD_SYSTEM ) ? GDT_KERNEL_DATA : GDT_USER_DATA | 3;
    Thread->TrapFrame.SegDs = Thread->TrapFrame.SegSs;
    Thread->TrapFrame.SegEs = Thread->TrapFrame.SegSs;
    Thread->TrapFrame.SegFs = Thread->TrapFrame.SegSs;
    Thread->TrapFrame.Dr6 = 0xFFFF0FF0;
    Thread->TrapFrame.Dr7 = 0x400;

    Thread->ThreadState = THREAD_STATE_NOT_READY;
    Thread->PreviousMode = ( ( Flags & THREAD_SYSTEM ) == THREAD_SYSTEM ) ? KernelMode : UserMode;

    Processor = KeQueryProcessorByNumber( Thread->ProcessorNumber );

    Thread->TrapFrame.SegGs = Processor->SegGs;

    KeAcquireSpinLock( &Processor->ThreadQueueLock, &PreviousIrql );

    KeInsertTailList( &Processor->ThreadQueue->ThreadQueue, &Thread->ThreadQueue );
    Processor->ThreadQueueLength++;

    if ( Process->ThreadCount == 0 ) {
        KeInitializeHeadList( &Thread->ThreadLinks );
        Process->ThreadLinks = &Thread->ThreadLinks;
    }
    else {
        KeInsertTailList( Process->ThreadLinks, &Thread->ThreadLinks );
    }
    Process->ThreadCount++;

    KeReleaseSpinLock( &Processor->ThreadQueueLock, PreviousIrql );

    if ( ( Flags & THREAD_SUSPENDED ) == 0 ) {

        Thread->ThreadState = THREAD_STATE_READY;
    }
    else {

        Thread->SuspendCount++;
    }
    KeReleaseSpinLock( &Thread->ThreadLock, PreviousIrql );

    if ( ThreadId != NULL ) {

        *ThreadId = Thread->ThreadId;
    }

    KiEnsureProcessorReady( Thread->ProcessorNumber );

    return STATUS_SUCCESS;
}

//
// lol dont call this, its bugged, use ZwCreateThread with THREAD_SYSTEM parameter.
//

NTSTATUS
ZwCreateSystemThread(
    _Out_ PHANDLE         ThreadHandle,
    _In_  ACCESS_MASK     DesiredAccess,
    _In_  PKSTART_ROUTINE ThreadStart,
    _In_  PVOID           ThreadContext,
    _In_  ULONG32         Flags,
    _In_  ULONG64         StackLength
)
{
    NTSTATUS ntStatus;
    HANDLE ProcessHandle;
    OBJECT_ATTRIBUTES ThreadAttributes = { { 0 }, { 0 }, OBJ_KERNEL_HANDLE };

    //
    // This function is currently bugged?
    //

    ntStatus = ObOpenObjectFromPointer( &ProcessHandle,
                                        PsInitialSystemProcess,
                                        PROCESS_ALL_ACCESS,
                                        OBJ_KERNEL_HANDLE,
                                        KernelMode );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    Flags |= THREAD_SYSTEM;
    ntStatus = ZwCreateThread( ThreadHandle,
                               ProcessHandle,
                               DesiredAccess,
                               ThreadStart,
                               ThreadContext,
                               Flags,
                               &ThreadAttributes,
                               StackLength,
                               NULL );
    ZwClose( ProcessHandle );

    return ntStatus;
}

VOID
PsTerminateThread(
    _In_ PKTHREAD Thread,
    _In_ NTSTATUS ExitCode
)
{
    KIRQL PreviousIrql;
    PKTHREAD CurrentThread;

    KeAcquireSpinLock( &Thread->ThreadLock, &PreviousIrql );

    CurrentThread = PsGetCurrentThread( );

    Thread->ExitCode = ExitCode;
    Thread->ThreadState = THREAD_STATE_TERMINATING;

    Thread->Process->ThreadCount--;
    KeRemoveList( &Thread->ThreadLinks );
    KeReleaseSpinLock( &Thread->ThreadLock, PreviousIrql );

    if ( Thread == CurrentThread ) {

        while ( TRUE ) {

            KiLeaveQuantumEarly( );
        }
    }
}

NTSTATUS
ZwTerminateThread(
    _In_ HANDLE   ThreadHandle,
    _In_ NTSTATUS ExitCode
)
{
    NTSTATUS ntStatus;
    PKTHREAD Thread;

    ntStatus = ObReferenceObjectByHandle( &Thread,
                                          ThreadHandle,
                                          THREAD_TERMINATE,
                                          KernelMode,
                                          PsThreadObject );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    if ( Thread == PsGetCurrentThread( ) ) {

        ObDereferenceObject( Thread );
    }

    PsTerminateThread( Thread, ExitCode );
    if ( Thread != PsGetCurrentThread( ) ) { // lol, it will never get here if this condition is not satisfied.

        ObDereferenceObject( Thread );
    }

    return STATUS_SUCCESS;
}

VOID
PsSuspendThread(
    _In_      PKTHREAD Thread,
    _Out_opt_ PULONG64 SuspendCount
)
{
    //
    // Implement PsResumeThread and more ps support
    // procedures for object management, instead of
    // direct syscalls.
    //

    KIRQL PreviousIrql;
    PKPCB Processor;

    KeAcquireSpinLock( &Thread->ThreadLock, &PreviousIrql );
    if ( SuspendCount ) {

        *SuspendCount = Thread->SuspendCount;
    }

    Thread->SuspendCount++;
    if ( Thread->ThreadState == THREAD_STATE_READY ) {

        Thread->ThreadState = THREAD_STATE_NOT_READY;
    }
    KeReleaseSpinLock( &Thread->ThreadLock, PreviousIrql );

    //
    // This is new, not properly tested.
    // attempts to wait for the thread to stop executing,
    // you could potentially issue and ipi for the scheduler
    // interrupt handler, causing another thread to be scheduled.
    //

    Processor = KeQueryProcessorByNumber( Thread->ProcessorNumber );

    while ( Processor->ThreadQueue == Thread )
        ;

}

NTSTATUS
ZwSuspendThread(
    _In_      HANDLE   ThreadHandle,
    _Out_opt_ PULONG64 SuspendCount
)
{
    NTSTATUS ntStatus;
    PKTHREAD Thread;

    ntStatus = ObReferenceObjectByHandle( &Thread,
                                          ThreadHandle,
                                          THREAD_SUSPEND_RESUME,
                                          KernelMode,
                                          PsThreadObject );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    if ( Thread == PsGetCurrentThread( ) ) {

        ObDereferenceObject( Thread );
        return STATUS_INVALID_THREAD;
    }

    PsSuspendThread( Thread, SuspendCount );

    ObDereferenceObject( Thread );
    return STATUS_SUCCESS;
}

VOID
PsResumeThread(
    _In_      PKTHREAD Thread,
    _Out_opt_ PULONG64 SuspendCount
)
{
    KIRQL PreviousIrql;

    KeAcquireSpinLock( &Thread->ThreadLock, &PreviousIrql );
    if ( SuspendCount ) {

        *SuspendCount = Thread->SuspendCount;
    }

    if ( Thread->SuspendCount == 0 ) {

        KeReleaseSpinLock( &Thread->ThreadLock, PreviousIrql );
        return;
    }

    Thread->SuspendCount--;
    if ( Thread->SuspendCount == 0 &&
         Thread->ThreadState == THREAD_STATE_NOT_READY ) {

        Thread->ThreadState = THREAD_STATE_READY;
        KiEnsureProcessorReady( Thread->ProcessorNumber );
    }
    KeReleaseSpinLock( &Thread->ThreadLock, PreviousIrql );
}

NTSTATUS
ZwResumeThread(
    _In_      HANDLE   ThreadHandle,
    _Out_opt_ PULONG64 SuspendCount
)
{
    NTSTATUS ntStatus;
    PKTHREAD Thread;

    ntStatus = ObReferenceObjectByHandle( &Thread,
                                          ThreadHandle,
                                          THREAD_SUSPEND_RESUME,
                                          KernelMode,
                                          PsThreadObject );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    if ( Thread == PsGetCurrentThread( ) ) {

        ObDereferenceObject( Thread );
        return STATUS_INVALID_THREAD;
    }

    PsResumeThread( Thread, SuspendCount );

    ObDereferenceObject( Thread );
    return STATUS_SUCCESS;
}

NTSTATUS
ZwSetContextThread(
    _In_ HANDLE   ThreadHandle,
    _In_ PCONTEXT Context
)
{
    NTSTATUS ntStatus;
    PKTHREAD Thread;
    KIRQL PreviousIrql;

    ntStatus = ObReferenceObjectByHandle( &Thread,
                                          ThreadHandle,
                                          THREAD_SET_CONTEXT,
                                          KernelMode,
                                          PsThreadObject );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    if ( Thread == PsGetCurrentThread( ) ) {

        ObDereferenceObject( Thread );
        return STATUS_INVALID_THREAD;
    }

    if ( Thread->SuspendCount == 0 ) {

        ObDereferenceObject( Thread );
        return STATUS_INVALID_THREAD;
    }

    KeAcquireSpinLock( &Thread->ThreadLock, &PreviousIrql );
    RtlContextToTrapFrame( &Thread->TrapFrame, Context );
    KeReleaseSpinLock( &Thread->ThreadLock, PreviousIrql );

    ObDereferenceObject( Thread );
    return STATUS_SUCCESS;
}

NTSTATUS
ZwGetContextThread(
    _In_ HANDLE   ThreadHandle,
    _In_ PCONTEXT Context
)
{
    NTSTATUS ntStatus;
    PKTHREAD Thread;
    KIRQL PreviousIrql;

    ntStatus = ObReferenceObjectByHandle( &Thread,
                                          ThreadHandle,
                                          THREAD_GET_CONTEXT,
                                          KernelMode,
                                          PsThreadObject );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    if ( Thread == PsGetCurrentThread( ) ) {

        ObDereferenceObject( Thread );
        return STATUS_INVALID_THREAD;
    }

    if ( Thread->SuspendCount == 0 ) {

        ObDereferenceObject( Thread );
        return STATUS_INVALID_THREAD;
    }

    KeAcquireSpinLock( &Thread->ThreadLock, &PreviousIrql );
    RtlTrapFrameToContext( &Thread->TrapFrame, Context );
    KeReleaseSpinLock( &Thread->ThreadLock, PreviousIrql );

    ObDereferenceObject( Thread );
    return STATUS_SUCCESS;
}

NORETURN
VOID
ZwContinue(
    _In_ PCONTEXT Context
)
{
    KIRQL PreviousIrql;
    PKTHREAD Thread;

    //
    // This doesn't work, the scheduler saves
    // the registers for current.
    //

    Thread = PsGetCurrentThread( );
    KeAcquireSpinLock( &Thread->ThreadLock, &PreviousIrql );
    RtlContextToTrapFrame( &Thread->TrapFrame, Context );
    KeQueryCurrentProcessor( )->SaveThread = FALSE; // hack
    Thread->ThreadState = THREAD_STATE_READY;
    KeReleaseSpinLock( &Thread->ThreadLock, PreviousIrql );

    while ( TRUE ) {

        KiLeaveQuantumEarly( );
    }
}

BOOLEAN
PspThreadIdQuery(
    _In_ POBJECT_TYPE ThreadType,
    _In_ PKTHREAD     Thread,
    _In_ PVOID        Context
)
{
    ThreadType;

    struct _PS_QUERY_CONTEXT {
        ULONG64  ThreadId;
        PKTHREAD Thread;
    } *PsQueryContext;

    PsQueryContext = Context;

    if ( PsQueryContext->ThreadId == Thread->ThreadId ) {

        PsQueryContext->Thread = Thread;
        return FALSE;
    }

    return TRUE;
}

NTSTATUS
ZwOpenThread(
    _Out_ PHANDLE            ThreadHandle,
    _In_  ACCESS_MASK        DesiredAccess,
    _In_  POBJECT_ATTRIBUTES ObjectAttributes,
    _In_  ULONG64            ThreadId
)
{
    NTSTATUS ntStatus;
    struct _PS_QUERY_CONTEXT {
        ULONG64  ThreadId;
        PKTHREAD Thread;
    } PsQueryContext;

    if ( ObjectAttributes->ObjectName.Buffer == NULL ) {

        PsQueryContext.ThreadId = ThreadId;
        PsQueryContext.Thread = NULL;

        ObQueryObjectList( PsThreadObject, ( PQUERY_PROCEDURE )PspThreadIdQuery, &PsQueryContext );

        if ( PsQueryContext.Thread == NULL ) {

            return STATUS_INVALID_THREAD;
        }

        return ObOpenObjectFromPointer( ThreadHandle,
                                        PsQueryContext.Thread,
                                        DesiredAccess,
                                        ObjectAttributes->Attributes,
                                        KernelMode );
    }
    else {

        ntStatus = ObReferenceObjectByName( &PsQueryContext.Thread,
                                            &ObjectAttributes->ObjectName,
                                            PsThreadObject );
        if ( !NT_SUCCESS( ntStatus ) ) {

            return ntStatus;
        }

        return ObOpenObjectFromPointer( ThreadHandle,
                                        PsQueryContext.Thread,
                                        DesiredAccess,
                                        ObjectAttributes->Attributes,
                                        KernelMode );
    }
}

VOID
PspCleanupThread(
    _In_ PKTHREAD Thread
)
{
    HANDLE ProcessHandle;
    ObOpenObjectFromPointer( &ProcessHandle,
                             Thread->Process,
                             PROCESS_ALL_ACCESS,
                             OBJ_KERNEL_HANDLE,
                             KernelMode );

    PspDestroyStack( ProcessHandle,
        ( PVOID )Thread->StackBase,
                     Thread->StackLength,
                     Thread->Process != PsInitialSystemProcess );

    if ( Thread->KernelStackLength != 0 ) {

        PspDestroyStack( 0, ( PVOID )Thread->KernelStackBase, Thread->KernelStackLength, FALSE );
    }

    ZwClose( ProcessHandle );
}

VOID
PsSetThreadProcessor(
    _In_ PKTHREAD Thread,
    _In_ ULONG64  ProcessorNumber
)
{
    PKPCB OldProcessor;
    PKPCB NewProcessor;
    KIRQL PreviousIrql;

    //
    // This function is mainly for thread pool stuff,
    // implement functions to switch the current thread
    // processor.
    //

    if ( Thread == PsGetCurrentThread( ) ||
         Thread->ProcessorNumber == ProcessorNumber ) {

        return;
    }

    PsSuspendThread( Thread, NULL );

    OldProcessor = KeQueryProcessorByNumber( Thread->ProcessorNumber );
    NewProcessor = KeQueryProcessorByNumber( ProcessorNumber );

    KeAcquireSpinLock( &OldProcessor->ThreadQueueLock, &PreviousIrql );
    KeAcquireSpinLockAtDpcLevel( &NewProcessor->ThreadQueueLock );

    OldProcessor->ThreadQueueLength--;
    NewProcessor->ThreadQueueLength++;

    KeRemoveList( &Thread->ThreadLinks );
    KeInsertTailList( &NewProcessor->ThreadQueue->ThreadQueue, &Thread->ThreadLinks );
    Thread->ProcessorNumber = ProcessorNumber;

    KeReleaseSpinLockAtDpcLevel( &OldProcessor->ThreadQueueLock );
    KeReleaseSpinLock( &NewProcessor->ThreadQueueLock, PreviousIrql );

    PsResumeThread( Thread, NULL );
}

NTSTATUS
NtCreateThread(
    _Out_     PHANDLE            ThreadHandle,
    _In_      HANDLE             ProcessHandle,
    _In_      ACCESS_MASK        DesiredAccess,
    _In_      PKSTART_ROUTINE    ThreadStart,
    _In_      PVOID              ThreadContext,
    _In_      ULONG32            Flags,
    _In_      POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_  ULONG64            StackLength,
    _Out_opt_ PULONG64           ThreadId
)
{
    __try {

        return ZwCreateThread( ThreadHandle,
                               ProcessHandle,
                               DesiredAccess,
                               ThreadStart,
                               ThreadContext,
                               Flags,
                               ObjectAttributes,
                               StackLength,
                               ThreadId );
    }
    __except ( EXCEPTION_EXECUTE_HANDLER ) {

        return STATUS_UNSUCCESSFUL;
    }
}
