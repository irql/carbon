


#include <carbsup.h>
#include "halp.h"
#include "../ke/ki.h"
#include "../mm/mi.h"
#include "../rtl/rtlp.h"
#include "../ps/psp.h"

VOLATILE ULONG64 KiCurrentInitCpu = 0;
VOLATILE ULONG64 KiCpuInitComplete = 0;

VOID
HalEnableCpuFeatures(

)
{
    //__writecr4( __readcr4( ) | ( 1 << 7 ) ); // pge.
    __writemsr( IA32_MSR_EFER, __readmsr( IA32_MSR_EFER ) | ( 1 << 11 ) );
}

VOID
HalInitializeCpu0(

)
{
    PKPCB Processor;
    PKTASK_STATE TaskState;

    KGDT_CODE_SEGMENT KernelCode64 = { 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 0 };
    KGDT_CODE_SEGMENT KernelData = { 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0 };
    KGDT_CODE_SEGMENT UserCode64 = { 0, 0, 0, 0, 1, 0, 1, 1, 3, 1, 0, 0, 1, 0, 1, 0 };
    KGDT_CODE_SEGMENT UserData = { 0, 0, 0, 0, 1, 0, 0, 1, 3, 1, 0, 0, 0, 0, 0, 0 };
    KGDT_CODE_SEGMENT UserGsBase = { 0, 0, 0, 0, 1, 0, 0, 1, 3, 1, 0, 0, 0, 0, 0, 0 };

    KiInitializeServiceCallTable( );

    Processor = KiCreatePcb( );
    Processor->ApicId = HalLocalApics[ 0 ]->ApicId;

    HalCreateInterrupt( MmAllocatePoolWithTag( NonPagedPoolZeroed,
                                               sizeof( KIDT_GATE[ 256 ] ),
                                               HAL_TAG ), &Processor->Interrupt );

    __lidt( &Processor->Interrupt );
    _enable( );

    HalCreateGlobal( &Processor->Global );

    HalInsertCodeSegment( &Processor->Global, &KernelCode64 );
    HalInsertCodeSegment( &Processor->Global, &KernelData );
    HalInsertCodeSegment( &Processor->Global, &UserData );
    HalInsertCodeSegment( &Processor->Global, &UserCode64 );
    Processor->SegGs = HalInsertCodeSegment( &Processor->Global, &UserGsBase );

    TaskState = MmAllocatePoolWithTag( NonPagedPoolZeroed, sizeof( KTASK_STATE ), HAL_TAG );
    TaskState->Ist[ 0 ] = ( ULONG64 )PspCreateStack( 0, 0x8000 );
    TaskState->Ist[ 1 ] = ( ULONG64 )PspCreateStack( 0, 0x8000 );
    //TaskState->Rsp0 =  ( ULONG64 )PspCreateStack( 0, 0x8000 );
    TaskState->IopbOffset = sizeof( KTASK_STATE );
    Processor->TaskStateDescriptor = HalInsertTaskSegment( &Processor->Global,
                                                           TaskState,
                                                           sizeof( KTASK_STATE ) );

    _lgdt( &Processor->Global );
    __ltr( ( USHORT )Processor->TaskStateDescriptor );

    __writemsr( IA32_MSR_GS_KERNEL_BASE, ( ULONG64 )Processor );

    HalLocalApicEnable( );
}

VOID
HalProcessorStartupPrepare(

)
{
    PKPCB Processor;
    PKPCB Processor0;
    PKTASK_STATE TaskState;
    KGDT_CODE_SEGMENT UserGsBase = { 0, 0, 0, 0, 1, 0, 0, 1, 3, 1, 0, 0, 0, 0, 0, 0 };

    HalEnableCpuFeatures( );
    MmInitializeCaching( );
    KiInitializeServiceCallTable( );

    Processor = KiCreatePcb( );
    Processor->ApicId = KiCurrentInitCpu;
    KiCurrentInitCpu = 0;

    Processor0 = KeQueryProcessorByNumber( 0 );

    RtlCopyMemory( &Processor->Interrupt, &Processor0->Interrupt, sizeof( KDESCRIPTOR_TABLE ) );
    RtlCopyMemory( &Processor->Global, &Processor0->Global, sizeof( KDESCRIPTOR_TABLE ) );

    __lidt( &Processor->Interrupt );
    _enable( );

    TaskState = MmAllocatePoolWithTag( NonPagedPoolZeroed, sizeof( KTASK_STATE ), HAL_TAG );
    TaskState->Ist[ 0 ] = ( ULONG64 )PspCreateStack( 0, 0x8000 );
    TaskState->Ist[ 1 ] = ( ULONG64 )PspCreateStack( 0, 0x8000 );
    //TaskState->Rsp0 =  ( ULONG64 )PspCreateStack( 0, 0x8000 );
    TaskState->IopbOffset = sizeof( KTASK_STATE );
    Processor->TaskStateDescriptor = HalInsertTaskSegment( &Processor->Global,
                                                           TaskState,
                                                           sizeof( KTASK_STATE ) );

    Processor->SegGs = HalInsertCodeSegment( &Processor->Global, &UserGsBase );

    _lgdt( &Processor->Global );
    __ltr( ( USHORT )Processor->TaskStateDescriptor );

    __writemsr( IA32_MSR_GS_KERNEL_BASE, ( ULONG64 )Processor );

    HalLocalApicEnable( );
    _InterlockedIncrement64( ( LONG64* )&KiCpuInitComplete );
    KeLowerIrql( PASSIVE_LEVEL );
    while ( 1 ) {

        __halt( );
    }
}

VOID
HalStartProcessors(

)
{

    ULONG64 ProcessorNumber;
    ULONG64 TimeCounter;
    PUCHAR ExecBase = ( PUCHAR )( 0x1000 );
    PKSTARTUP ExecStartup = ( PKSTARTUP )( 0x1000 + ( CHAR* )HalProcessorStartupEnd - ( CHAR* )HalProcessorStartup );

    RtlCopyMemory( ExecBase, ( PVOID )HalProcessorStartup, ( ULONG )( ( CHAR* )HalProcessorStartupEnd - ( CHAR* )HalProcessorStartup ) );
    ExecStartup->TableBase = ( ULONG32 )__readcr3( );
    ExecStartup->GdtBase = 0x1800;
    ExecStartup->GdtLimit = 23;
    ExecStartup->InitialJmp = ( ULONG64 )HalProcessorStartupPrepare;

    for ( ProcessorNumber = 1, TimeCounter = 0; ProcessorNumber < HalLocalApicCount; ProcessorNumber++ ) {

        KiCurrentInitCpu = HalLocalApics[ ProcessorNumber ]->ApicId;
        ExecStartup->InitialRsp = ( ULONG64 )MmAllocatePoolWithTag( NonPagedPoolExecute, 0x4000, KE_TAG ) + 0x4000; // no nx bit.
        HalLocalApicSendIpi( HalLocalApics[ ProcessorNumber ]->ApicId, LAPIC_MT_INIT );
        HalDelayExecutionPit( 10 );
        HalLocalApicSendIpi( HalLocalApics[ ProcessorNumber ]->ApicId, LAPIC_MT_START_UP | 0x01 );
        while ( TimeCounter != 10 && KiCurrentInitCpu != 0 ) {
            HalDelayExecutionPit( 10 );
            TimeCounter++;
        }

        if ( KiCurrentInitCpu != 0 ) {

            HalLocalApicSendIpi( HalLocalApics[ ProcessorNumber ]->ApicId, LAPIC_MT_START_UP | 0x01 );
            TimeCounter = 0;

            while ( TimeCounter < 100 && KiCurrentInitCpu != 0 ) {
                HalDelayExecutionPit( 10 );
                TimeCounter++;
            }

            if ( KiCurrentInitCpu != 0 ) {

                RtlDebugPrint( L"processor startup sequence failed for %ul\n", ProcessorNumber );
                continue;
            }
        }
    }
}
