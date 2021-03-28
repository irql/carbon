


#include <carbsup.h>
#include "../hal/halp.h"
#include "ki.h"
#include "../ps/psp.h"
#include "../mm/mi.h"

VOID
KiIdleThread(

)
{
    //RtlDebugPrint( L"idle start. %d\n", KeQueryCurrentProcessor( )->ProcessorNumber );

    while ( 1 ) {
        KiSleepIdleProcessor( );
        //KiLeaveQuantumEarly( );
    }
}

VOID
KeInitializeKernelCore(

)
{
    STATIC OBJECT_ATTRIBUTES IdleAttributes = { 0 };

    PKPCB Processor;
    PKTHREAD Thread;
    ULONG64 ProcessorNumber;

    for ( ProcessorNumber = 0; ProcessorNumber < KeQueryProcessorCount( ); ProcessorNumber++ ) {

        Processor = KeQueryProcessorByNumber( ProcessorNumber );
        ObCreateObject( &Thread, PsThreadObject, &IdleAttributes, sizeof( KTHREAD ) );

        Thread->StackLength = 0x4000;
        Thread->StackBase = ( ULONG64 )PspCreateStack( 0, Thread->StackLength );

        Thread->ThreadId = KeGenerateUniqueId( );
        Thread->TrapFrame.Cr3 = __readcr3( );
        Thread->TrapFrame.EFlags = 0x202;
        Thread->TrapFrame.Rip = ( ULONG64 )KiIdleThread;
        Thread->TrapFrame.Rsp = Thread->StackBase + Thread->StackLength - 0x28;
        Thread->TrapFrame.SegCs = GDT_KERNEL_CODE64;
        Thread->TrapFrame.SegSs = GDT_KERNEL_DATA;
        Thread->TrapFrame.SegDs = GDT_KERNEL_DATA;
        Thread->TrapFrame.Dr6 = 0xffff0ff0;
        Thread->TrapFrame.Dr7 = 0x400;
        Thread->ThreadState = THREAD_STATE_IDLE;
        Thread->ProcessorNumber = ProcessorNumber;
        Thread->Process = PsInitialSystemProcess;

        Processor->ThreadQueueLock = 0;
        Processor->ThreadQueueLength++;
        Processor->ThreadQueue = Thread;

        KeInitializeHeadList( &Thread->ThreadQueue );

        if ( ProcessorNumber == 0 ) {
            KeInitializeHeadList( &Thread->ThreadLinks );
            PsInitialSystemProcess->ThreadLinks = &Thread->ThreadLinks;
        }
        else {
            KeInsertTailList( PsInitialSystemProcess->ThreadLinks, &Thread->ThreadLinks );
        }

        PsInitialSystemProcess->ThreadCount++;
    }
}

VOID
KiDpcThread(
    _In_ PKPCB Processor
)
{
    PKDPC Dpc;
    KIRQL PreviousIrql;

    while ( TRUE ) {

        if ( Processor->DpcQueueLock == 0 &&
             Processor->DpcQueueLength > 0 ) {

            Dpc = Processor->DpcQueue;
            KeRaiseIrql( Dpc->DeferredIrql, &PreviousIrql );
            MiSetAddressSpace( Dpc->DirectoryTableBase );
            Dpc->DeferredRoutine( Dpc, Dpc->DeferredContext );
            KeLowerIrql( PreviousIrql );
            Dpc->Completed = TRUE;

            Processor->DpcQueue = Dpc->DpcLink;
            Processor->DpcQueueLength--;
            //MmFreePoolWithTag( Dpc, KE_TAG );
        }
    }
}

VOID
KePhase1InitializeKernelCore(

)
{
    PKPCB Processor;
    ULONG64 ProcessorNumber;

    HANDLE ThreadHandle;
    PKTHREAD Thread;
    OBJECT_ATTRIBUTES ThreadAttributes = { { 0 }, { 0 }, OBJ_KERNEL_HANDLE };

    for ( ProcessorNumber = 0; ProcessorNumber < KeQueryProcessorCount( ); ProcessorNumber++ ) {

        Processor = KeQueryProcessorByNumber( ProcessorNumber );

        ZwCreateThread( &ThreadHandle,
                        ZwCurrentProcess( ),
                        THREAD_ALL_ACCESS,
                        ( PKSTART_ROUTINE )KiDpcThread,
                        Processor,
                        THREAD_SYSTEM | THREAD_SUSPENDED,
                        &ThreadAttributes,
                        0,
                        NULL );
        ObReferenceObjectByHandle( &Thread,
                                   ThreadHandle,
                                   0,
                                   KernelMode,
                                   PsThreadObject );
        PsSetThreadProcessor( Thread, ProcessorNumber );
        PsResumeThread( Thread, NULL );

        ObDereferenceObject( Thread );
        ZwClose( ThreadHandle );
    }
}
