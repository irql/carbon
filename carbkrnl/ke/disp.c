


#include <carbsup.h>
#include "../hal/halp.h"
#include "ki.h"
#include "../mm/mi.h"
#include "../ps/psp.h"
#include "../ob/obp.h"
#include "../io/iop.h"

KSPIN_LOCK    KiDispatcherLock = { 1 };
PKIPI_SERVICE KiIpiService;
KSPIN_LOCK    KiIpiLock = { 0 };

#define TEST_EVENT( dpc_object )        ( ( dpc_object )->Type == DPC_OBJECT_EVENT && ( ( PKEVENT )( dpc_object ) )->Signaled )
#define TEST_THREAD( dpc_object )       ( ( dpc_object )->Type == DPC_OBJECT_MUTEX && _InterlockedCompareExchange64( ( volatile long long* )&( ( PKMUTEX )( dpc_object ) )->Owner, ( long long )Thread, 0 ) == 0 )
#define TEST_MUTEX( dpc_object )        ( ( dpc_object )->Type == DPC_OBJECT_THREAD && ( ( PKTHREAD )( dpc_object ) )->ThreadState == THREAD_STATE_TERMINATED )
#define TEST_SEMAPHORE( dpc_object )    ( ( dpc_object )->Type == DPC_OBJECT_SEMAPHORE && ( ( PKSEMAPHORE )( dpc_object ) )->Count < ( ( PKSEMAPHORE )( dpc_object ) )->Limit )
#define RESET_THREAD_STATE( thread )                    \
Thread->WaitTimeout = 0;                                \
if ( ( thread )->SuspendCount != 0 ) {                  \
                                                        \
    ( thread )->ThreadState = THREAD_STATE_NOT_READY;   \
} else {                                                \
                                                        \
    ( thread )->ThreadState = THREAD_STATE_READY;       \
}

VOID
KiSwapContext(
    _In_ PKTRAP_FRAME TrapFrame
)
{
    PKPCB Processor;
    PKTHREAD Thread;
    PKTHREAD LastThread;
    PKDPC_HEADER Header;
    BOOLEAN SkipThread;
    PLIST_ENTRY Flink;
    KIRQL PreviousIrql;

    KeRaiseIrql( DISPATCH_LEVEL, &PreviousIrql );

    Processor = KeQueryCurrentProcessor( );

    if ( KeQuerySpinLock( &KiDispatcherLock ) ||
         Processor == NULL ) {

        KeLowerIrql( PreviousIrql );
        return;
    }

    Thread = Processor->ThreadQueue;
    LastThread = Thread;

    Thread->ProcessorTime += 10;
    Processor->TickCount += 10;

    if ( KeQuerySpinLock( &Processor->ThreadQueueLock ) ) {

        KeLowerIrql( PreviousIrql );
        return;
    }

    if ( Processor->SaveThread ) {

        RtlCopyMemory( &Thread->TrapFrame, TrapFrame, sizeof( KTRAP_FRAME ) );

        if ( Thread->ThreadState == THREAD_STATE_RUNNING ) {

            RESET_THREAD_STATE( Thread );
        }
    }

    if ( !Processor->SaveThread ) {

        Processor->SaveThread = TRUE;
    }

    //
    // Start at the next thread in the queue
    //

    Flink = LastThread->ThreadQueue.Flink;
    do {
        Thread = CONTAINING_RECORD( Flink, KTHREAD, ThreadQueue );
        Flink = Flink->Flink;
        SkipThread = FALSE;

        if ( !KeQuerySpinLock( &Thread->ThreadLock ) ) {

            switch ( Thread->ThreadState ) {
            case THREAD_STATE_TERMINATING:;

                Thread->ThreadState = THREAD_STATE_TERMINATED;
                /*
                KeRemoveList( &Thread->ThreadQueue );
                Processor->ThreadQueueLength--;

                KiEnsureAllProcessorsReady( );

                //
                // This is incase the thread queue ends here, we skip it incase it was the last
                // and then iterate to the next so the value of Thread is safe.
                //

                Thread = CONTAINING_RECORD( Flink, KTHREAD, ThreadQueue );*/
                SkipThread = TRUE;
                break;
            case THREAD_STATE_WAITING:;
                Header = ( PKDPC_HEADER )Thread->WaitObject;

                if ( Thread->WaitTimeout != WAIT_TIMEOUT_INFINITE ) {

                    if ( Thread->WaitTimeout <= Processor->TickCount ) {

                        RESET_THREAD_STATE( Thread );
                    }
                }

                if ( Header != NULL &&
                     Thread->WaitTimeout != 0 ) {

                    switch ( Header->Type ) {
                    case DPC_OBJECT_EVENT:;

                        if ( ( ( PKEVENT )Header )->Signaled ) {

                            RESET_THREAD_STATE( Thread );
                        }
                        break;
                    case DPC_OBJECT_THREAD:;

                        if ( ( ( PKTHREAD )Header )->ThreadState == THREAD_STATE_TERMINATED ) {

                            RESET_THREAD_STATE( Thread );
                        }
                        break;
                    case DPC_OBJECT_MUTEX:;
                        if ( _InterlockedCompareExchange64(
                            ( volatile long long* )&( ( PKMUTEX )Header )->Owner,
                            ( long long )Thread, 0 ) == 0 ) {

                            RESET_THREAD_STATE( Thread );
                        }
                        break;
                    case DPC_OBJECT_SEMAPHORE:;
                        if ( ( ( PKSEMAPHORE )Header )->Count < ( ( PKSEMAPHORE )Header )->Limit ) {

                            // maybe spin lock these lol
                            _InterlockedIncrement64( &( ( PKSEMAPHORE )Header )->Count );
                            RESET_THREAD_STATE( Thread );
                        }
                        break;
                    default:

                        NT_ASSERT( FALSE );
                    }
                }

                break;
            case THREAD_STATE_IDLE:;

                SkipThread = TRUE;
                break;
            default:
                break;
            }

        }
        else {

            SkipThread = TRUE;
        }

        if ( SkipThread ) {

            continue;
        }

        if ( Thread->ThreadState == THREAD_STATE_READY ) {

            break;
        }

    } while ( Thread != LastThread );



    if ( Thread->ThreadState != THREAD_STATE_READY &&
         Thread->ThreadState != THREAD_STATE_IDLE &&
         !SkipThread ) {

        if ( LastThread->ThreadState == THREAD_STATE_READY ) {

            Thread = LastThread;
        }
        else {

            do {
                Thread = CONTAINING_RECORD( Flink, KTHREAD, ThreadQueue );
                Flink = Flink->Flink;

            } while ( Thread->ThreadState != THREAD_STATE_IDLE );
        }
    }

    RtlCopyMemory( TrapFrame, &Thread->TrapFrame, sizeof( KTRAP_FRAME ) );

    if ( Thread->ThreadState != THREAD_STATE_IDLE ) {

        Thread->ThreadState = THREAD_STATE_RUNNING;
    }
    Processor->ThreadQueue = Thread;

    // this arch is fucking stupid
    // so the IA32_MSR_GS_BASE is completely useless? and it's actually
    // the gdt segment base that's used (32 bits (? unless i can use S)), swapgs can be used
    // to have to gs base be set to IA32_MSR_KERNEL_GS_BASE, so only one of the two
    // msrs are actually used?
    //__writemsr( IA32_MSR_GS_BASE, ( unsigned long long )Thread->Process->Peb );
    HalSetCodeSegmentBase(
        ( PKGDT_CODE_SEGMENT )( Processor->Global.Base + Processor->SegGs ),
        Thread->Process->Peb );
    Processor->TaskState.Rsp[ 0 ] = Thread->KernelStackBase + Thread->KernelStackLength;

    KeLowerIrql( PreviousIrql );
    return;
}

VOID
KeInitializeEvent(
    _In_ PKEVENT Event,
    _In_ BOOLEAN Signal
)
{
    Event->Header.Type = DPC_OBJECT_EVENT;
    Event->Signaled = Signal;
}

BOOLEAN
KeQueryEvent(
    _In_ PKEVENT Event
)
{
    return Event->Signaled;
}

VOID
KeSignalEvent(
    _In_ PKEVENT Event,
    _In_ BOOLEAN Signal
)
{
    Event->Signaled = Signal;
    KiEnsureAllProcessorsReady( ); // if anything is waiting, and a processor is sleeping.
}

NTSTATUS
ZwCreateEvent(
    _Out_ PHANDLE            EventHandle,
    _In_  BOOLEAN            EventSignal,
    _In_  POBJECT_ATTRIBUTES EventAttributes
)
{
    NTSTATUS ntStatus;
    PKEVENT EventObject;

    ntStatus = ObCreateObject( &EventObject,
                               KeEventObject,
                               EventAttributes,
                               sizeof( KEVENT ) );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    ntStatus = ObOpenObjectFromPointer( EventHandle,
                                        EventObject,
                                        0,
                                        EventAttributes->Attributes,
                                        KernelMode );
    if ( !NT_SUCCESS( ntStatus ) ) {

        ObDereferenceObject( EventObject );
        return ntStatus;
    }

    KeInitializeEvent( EventObject, EventSignal );
    ObDereferenceObject( EventObject );
    return STATUS_SUCCESS;
}

NTSTATUS
ZwQueryEvent(
    _In_  HANDLE   EventHandle,
    _Out_ PBOOLEAN EventSignal
)
{
    NTSTATUS ntStatus;
    PKEVENT EventObject;

    *EventSignal = FALSE;

    ntStatus = ObReferenceObjectByHandle( &EventObject,
                                          EventHandle,
                                          0,
                                          KernelMode,
                                          KeEventObject );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    *EventSignal = KeQueryEvent( EventObject );

    ObDereferenceObject( EventObject );
    return STATUS_SUCCESS;
}

NTSTATUS
ZwSignalEvent(
    _In_ HANDLE  EventHandle,
    _In_ BOOLEAN EventSignal
)
{
    NTSTATUS ntStatus;
    PKEVENT EventObject;

    ntStatus = ObReferenceObjectByHandle( &EventObject,
                                          EventHandle,
                                          0,
                                          KernelMode,
                                          KeEventObject );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    __try {

        KeSignalEvent( EventObject, EventSignal );
    }
    __except ( EXCEPTION_EXECUTE_HANDLER ) {

        ObDereferenceObject( EventObject );
        RtlRaiseException( STATUS_ACCESS_VIOLATION );
    }

    ObDereferenceObject( EventObject );
    return STATUS_SUCCESS;
}

// dont use dpcs, they are broken, just use ipis 
VOID
KeGenericCallDpc(
    _In_ PKDEFERRED_ROUTINE DeferredRoutine,
    _In_ PVOID              DeferredContext
)
{
    KDPC Dpc;
    ULONG64 Processor;

    KeInitializeDpc( &Dpc, ( PKDEFERRED_ROUTINE )DeferredRoutine, DeferredContext, LowPriority );

    Processor = 0;
    while ( Processor < KeQueryProcessorCount( ) ) {

        KeSetTargetProcessorDpc( &Dpc, Processor );
        KeInsertQueueDpc( &Dpc );

        while ( !Dpc.Completed )
            ;

        Processor++;
    }
}

VOID
KeInitializeDpc(
    _In_ PKDPC              Dpc,
    _In_ PKDEFERRED_ROUTINE DeferredRoutine,
    _In_ PVOID              DeferredContext,
    _In_ KPRIORITY          Priority
)
{
    Dpc->ProcessorNumber = PspGetThreadProcessor( );
    Dpc->DeferredRoutine = DeferredRoutine;
    Dpc->DeferredContext = DeferredContext;
    Dpc->DirectoryTableBase = MiGetAddressSpace( );
    Dpc->DeferredIrql = DISPATCH_LEVEL;
    Dpc->Priority = Priority;
    Dpc->DpcLink = NULL;
}

VOID
KeSetTargetProcessorDpc(
    _In_ PKDPC   Dpc,
    _In_ ULONG64 Number
)
{
    Dpc->ProcessorNumber = Number;
}

VOID
KeInsertQueueDpc(
    _In_ PKDPC Dpc
)
{
    PKPCB Processor;
    KIRQL PreviousIrql;
    PKDPC LastDpc;
    PKDPC CopyDpc;

    Processor = KeQueryProcessorByNumber( Dpc->ProcessorNumber );
    CopyDpc = MmAllocatePoolWithTag( NonPagedPool, sizeof( KDPC ), KE_TAG );
    RtlCopyMemory( CopyDpc, Dpc, sizeof( KDPC ) );

    KeAcquireSpinLock( &Processor->DpcQueueLock, &PreviousIrql );
    if ( Processor->DpcQueueLength == 0 ) {
        Processor->DpcQueue = CopyDpc;
        CopyDpc->DpcLink = NULL;
    }
    else {

        LastDpc = Processor->DpcQueue;
        while ( LastDpc->DpcLink != NULL ) {
            LastDpc = LastDpc->DpcLink;
        }
        LastDpc->DpcLink = CopyDpc;
        CopyDpc->DpcLink = NULL;
    }
    Processor->DpcQueueLength++;
    KeReleaseSpinLock( &Processor->DpcQueueLock, PreviousIrql );
    KiEnsureProcessorReady( Processor->ProcessorNumber );
}

VOID
KiLeaveQuantumEarly(

)
{
    NT_ASSERT( KeGetCurrentIrql( ) < DISPATCH_LEVEL );

    HalLocalApicWrite( LAPIC_CURRENT_COUNT_REGISTER, 0 );

    while ( PsGetCurrentThread( )->ThreadState != THREAD_STATE_RUNNING )
        ;
}

NTSTATUS
KeWaitForSingleObject(
    _In_ PVOID   Object,
    _In_ ULONG64 Timeout
)
{

    NT_ASSERT( KeGetCurrentIrql( ) < DISPATCH_LEVEL );

    KIRQL PreviousIrql;
    PKTHREAD Thread;
    PKDPC_HEADER DpcObject;

    DpcObject = Object;
    Thread = PsGetCurrentThread( );

    if ( Object != NULL ) {
        if ( TEST_EVENT( DpcObject ) ||
             TEST_THREAD( DpcObject ) ||
             TEST_MUTEX( DpcObject ) ||
             TEST_SEMAPHORE( DpcObject ) ) {

            return STATUS_SUCCESS;
        }

        NT_ASSERT( DpcObject->Type == DPC_OBJECT_EVENT ||
                   DpcObject->Type == DPC_OBJECT_MUTEX ||
                   DpcObject->Type == DPC_OBJECT_THREAD ||
                   DpcObject->Type == DPC_OBJECT_SEMAPHORE );
    }

    KeAcquireSpinLock( &Thread->ThreadLock, &PreviousIrql );

    Thread->WaitObject = Object;
    if ( Timeout == WAIT_TIMEOUT_INFINITE ) {

        Thread->WaitTimeout = Timeout;
    }
    else {

        Thread->WaitTimeout = NtGetTickCount( ) + Timeout;
    }
    Thread->ThreadState = THREAD_STATE_WAITING;

    KeReleaseSpinLock( &Thread->ThreadLock, PreviousIrql );

    while ( Thread->WaitTimeout != 0 ) {

        KiLeaveQuantumEarly( );
    }

    if ( Object != NULL ) {
        if ( DpcObject->Type == DPC_OBJECT_EVENT &&
             KeQueryEvent( Object ) ) {

            return STATUS_SUCCESS;
        }

        if ( DpcObject->Type == DPC_OBJECT_MUTEX &&
            ( ( PKMUTEX )Object )->Owner == Thread ) {

            return STATUS_SUCCESS;
        }

        if ( DpcObject->Type == DPC_OBJECT_THREAD &&
            ( ( PKTHREAD )Object )->ThreadState == THREAD_STATE_TERMINATED ) {

            return STATUS_SUCCESS;
        }

        if ( DpcObject->Type == DPC_OBJECT_SEMAPHORE &&
            ( ( PKSEMAPHORE )Object )->Count == ( ( PKSEMAPHORE )Object )->Limit ) {

            return STATUS_SUCCESS;
        }
    }

    return STATUS_TIMEOUT;
}

NTSTATUS
ZwWaitForSingleObject(
    _In_ HANDLE  ObjectHandle,
    _In_ ULONG64 TimeOut
)
{
    NTSTATUS ntStatus;
    PKDPC_HEADER ObjectPointer;

    if ( ARGUMENT_PRESENT( ObjectHandle ) ) {

        ntStatus = ObReferenceObjectByHandle( &ObjectPointer,
                                              ObjectHandle,
                                              0,
                                              KernelMode,
                                              NULL );
        if ( !NT_SUCCESS( ntStatus ) ) {

            return ntStatus;
        }

        if ( ObpGetHeaderFromObject( ObjectPointer )->Type != KeEventObject &&
             ObpGetHeaderFromObject( ObjectPointer )->Type != KeMutexObject &&
             ObpGetHeaderFromObject( ObjectPointer )->Type != PsThreadObject ) {

            ObDereferenceObject( ObjectPointer );
            return ntStatus;
        }

        if ( ObpGetHeaderFromObject( ObjectPointer )->Type == PsThreadObject &&
            ( PKTHREAD )ObjectPointer == PsGetCurrentThread( ) ) {

            // :funny_lo:
            ObDereferenceObject( ObjectPointer );
            return ntStatus;
        }
    }
    else {

        ObjectPointer = NULL;
    }

    ntStatus = KeWaitForSingleObject( ObjectPointer, TimeOut );

    if ( ObjectPointer != NULL ) {

        ObDereferenceObject( ObjectPointer );
    }

    return ntStatus;
}

NTSTATUS
NtWaitForSingleObject(
    _In_ HANDLE  ObjectHandle,
    _In_ ULONG64 TimeOut
)
{
    __try {

        return ZwWaitForSingleObject( ObjectHandle,
                                      TimeOut );
    }
    __except ( EXCEPTION_EXECUTE_HANDLER ) {

        RtlRaiseException( STATUS_ACCESS_VIOLATION );
    }
}

BOOLEAN
KiTrapProcessorWakeup(
    _In_ PKINTERRUPT Interrupt
)
{
    Interrupt;
    //RtlDebugPrint( L"wakeup p\n" );
    return TRUE;
}

VOID
KiSleepIdleProcessor(

)
{
    // c-states? shut the fuck up my brain is damaged.
    //KIRQL PreviousIrql;
    //KeRaiseIrql( DISPATCH_LEVEL, &PreviousIrql );
    //KeLowerIrql( DISPATCH_LEVEL );

    __halt( );

    //KIRQL Pog;
    //KeRaiseIrql( IPI_LEVEL, &Pog );
    //KeLowerIrql( PreviousIrql );
}

VOID
KiEnsureProcessorReady(
    _In_ ULONG64 Number
)
{

    if ( KeQueryProcessorByNumber( Number )->ThreadQueue->ThreadState == THREAD_STATE_IDLE ) {

        //HalLocalApicSendIpi( ( ULONG32 )KeQueryProcessorByNumber( Number )->ApicId, 0x30 | LOCAL_APIC_CR0_DEST_NORMAL | LOCAL_APIC_CR0_INIT_DEASSERT );
    }

}

VOID
KiEnsureAllProcessorsReady(

)
{
    ULONG64 Processor;

    for ( Processor = 0; Processor < KeQueryProcessorCount( ); Processor++ ) {

        KiEnsureProcessorReady( Processor );
    }
}

VOID
KiInitializeIpiCall(

)
{
    //PIO_INTERRUPT IpiInterrupt;
    //OBJECT_ATTRIBUTES Interrupt = { 0 };

    KiIpiService = MmAllocatePoolWithTag( NonPagedPool,
                                          sizeof( KIPI_SERVICE ) * HalLocalApicCount,
                                          KE_TAG );
    /*
    IoConnectInterrupt( &IpiInterrupt,
                        KiIpiCall,
                        KiIpiService,
                        0xFE,
                        IPI_LEVEL,
                        &Interrupt );
    ObDereferenceObject( IpiInterrupt );*/
}

VOID
KeGenericCallIpi(
    _In_ PKIPI_CALL BroadcastFunction,
    _In_ PVOID      BroadcastContext
)
{
    PKPCB Processor;
    ULONG64 ProcessorNumber;
    KIRQL PreviousIrql;

    // change to use LAPIC all excluding self
    KeRaiseIrql( IPI_LEVEL, &PreviousIrql );
    //KeAcquireSpinLockAtDpcLevel( &KiIpiLock );

    for ( ProcessorNumber = 0; ProcessorNumber < KeQueryProcessorCount( ); ProcessorNumber++ ) {

        KiIpiService[ ProcessorNumber ].BroadcastFunction = BroadcastFunction;
        KiIpiService[ ProcessorNumber ].BroadcastContext = BroadcastContext;
        Processor = KeQueryProcessorByNumber( ProcessorNumber );

        if ( Processor != KeQueryCurrentProcessor( ) ) {

            HalLocalApicSendIpi( ( ULONG32 )Processor->ApicId, 0xFE | LAPIC_MT_FIXED );
        }

    }

    BroadcastFunction( BroadcastContext );
    //KeReleaseSpinLockAtDpcLevel( &KiIpiLock );
    KeLowerIrql( PreviousIrql );
}

VOID
KiIpiCall(
    _In_ PKTRAP_FRAME TrapFrame
)
{
    TrapFrame;

    ULONG64 ProcessorService;
    KIRQL PreviousIrql;
    BOOLEAN InterruptingService;
    ULONG64 PreviousService;

    KeRaiseIrql( IPI_LEVEL, &PreviousIrql );

    InterruptingService = KeQueryCurrentProcessor( )->InService;
    if ( InterruptingService ) {

        PreviousService = KeQueryCurrentProcessor( )->PreviousService;
    }

    KeQueryCurrentProcessor( )->InService = TRUE;
    KeQueryCurrentProcessor( )->PreviousService = TrapFrame->Interrupt;

    //
    // End of interrupt prologue.
    //

    ProcessorService = KeQueryCurrentProcessor( )->ProcessorNumber;

    KiIpiService[ ProcessorService ].BroadcastFunction(
        KiIpiService[ ProcessorService ].BroadcastContext );

    //
    // Start of interrupt epilogue.
    //

    KeQueryCurrentProcessor( )->InService = InterruptingService;
    if ( InterruptingService ) {

        KeQueryCurrentProcessor( )->PreviousService = InterruptingService;
    }

    KeLowerIrql( PreviousIrql );
}

NTSTATUS
NtCreateMutex(
    _Out_ PHANDLE MutexHandle,
    _In_  BOOLEAN InitialOwner
)
{
    // add objectattr and allowed named mutexes

    NTSTATUS ntStatus;
    PKMUTEX MutexObject;
    OBJECT_ATTRIBUTES MutexAttributes = { 0 };
    HANDLE MutexHandle1;

    ntStatus = ObCreateObject( &MutexObject,
                               KeMutexObject,
                               &MutexAttributes,
                               sizeof( KMUTEX ) );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    ntStatus = ObOpenObjectFromPointer( &MutexHandle1,
                                        MutexObject,
                                        0,
                                        0,
                                        UserMode );
    if ( !NT_SUCCESS( ntStatus ) ) {

        ObDereferenceObject( MutexObject );
        return ntStatus;
    }

    __try {

        *MutexHandle = MutexHandle1;
    }
    __except ( EXCEPTION_EXECUTE_HANDLER ) {

        ZwClose( MutexHandle1 );
        ObDereferenceObject( MutexObject );
        return STATUS_ACCESS_VIOLATION;
    }

    KeInitializeMutex( MutexObject );
    if ( InitialOwner ) {
        MutexObject->Owner = PsGetCurrentThread( );
    }
    ObDereferenceObject( MutexObject );
    return STATUS_SUCCESS;
}

NTSTATUS
NtReleaseMutex(
    _In_ HANDLE MutexHandle
)
{
    NTSTATUS ntStatus;
    PKMUTEX MutexObject;

    ntStatus = ObReferenceObjectByHandle( &MutexObject,
                                          MutexHandle,
                                          0,
                                          UserMode,
                                          KeMutexObject );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    if ( MutexObject->Owner != PsGetCurrentThread( ) ) {

        ObDereferenceObject( MutexObject );
        return STATUS_ACCESS_DENIED;
    }

    KeReleaseMutex( MutexObject );
    ObDereferenceObject( MutexObject );
    return STATUS_SUCCESS;
}

ULONG64
NtGetTickCount(

)
{
    return KeQueryCurrentProcessor( )->TickCount;
}

VOID
KeInitializeSemaphore(
    _In_ PKSEMAPHORE Semaphore,
    _In_ LONG64      Limit,
    _In_ LONG64      Count
)
{
    Semaphore->Header.Type = DPC_OBJECT_SEMAPHORE;
    Semaphore->Limit = Limit;
    Semaphore->Count = Count;
}

VOID
KeAcquireSemaphore(
    _In_ PKSEMAPHORE Semaphore
)
{

    if ( Semaphore->Count < Semaphore->Limit ) {

        _InterlockedIncrement64( &Semaphore->Count );
        return;
    }

    KeWaitForSingleObject( Semaphore, WAIT_TIMEOUT_INFINITE );
}

VOID
KeReleaseSemaphore(
    _In_ PKSEMAPHORE Semaphore
)
{
    _InterlockedExchangeAdd64( &Semaphore->Count, -1 );
}
