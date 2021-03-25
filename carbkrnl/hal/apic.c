


#include <carbsup.h>
#include "halp.h"

PRSDP_DESCRIPTOR2_0                     HalRsdp;
PRSDT_DESCRIPTOR                        HalRsdt;
PXSDT_DESCRIPTOR                        HalXsdt;
PMADT                                   HalMadt;

PMADT_PROCESSOR_LOCAL_APIC*             HalLocalApics;
PMADT_IO_APIC*                          HalIoApics;
PMADT_INTERRUPT_SOURCE_OVERRIDE_APIC*   HalIsoApics;
PMADT_NON_MASKABLE_INTERRUPT_APIC*      HalNmiApics;

ULONG                                   HalLocalApicCount;
ULONG                                   HalIoApicCount;
ULONG                                   HalIsoApicCount;
ULONG                                   HalNmiApicCount;

PMADT_LOCAL_APIC_ADDRESS_OVERRIDE       HalLocalApicAddressOverride;

PUCHAR                                  HalLocalApic;
PUCHAR                                  HalIoApic;

FORCEINLINE
UCHAR
HalGetSecondFromCMOS(

)
{
    __outbyte( 0x70, 0 );
    return __inbyte( 0x71 );
}

VOID
HalLocalApicEnable(

)
{
    //ULONG32 IdRegisters[ 4 ];

    if ( HalLocalApic == 0 ) {
        HalLocalApic = MmMapIoSpace( HalMadt->LocalApicAddress, 0x1000 );
        HalIoApic = MmMapIoSpace( HalIoApics[ 0 ]->IoApicAddress, 0x2000 );
    }

    HalLocalApicWrite( LAPIC_LVT_TIMER_REGISTER, LAPIC_MASKED );
    HalLocalApicWrite( LAPIC_LVT_PERFORMANCE_MONITORING_COUNTERS_REGISTER, LAPIC_MT_NMI );
    HalLocalApicWrite( LAPIC_LVT_LINT0_REGISTER, LAPIC_MASKED );
    HalLocalApicWrite( LAPIC_LVT_LINT1_REGISTER, LAPIC_MASKED );
    HalLocalApicWrite( LAPIC_TASK_PRIORITY_REGISTER, 0 );

    __writecr8( DISPATCH_LEVEL );

    __writemsr( IA32_MSR_APIC_BASE, __readmsr( IA32_MSR_APIC_BASE ) | ( 1 << 11 ) );

    HalLocalApicWrite( LAPIC_SPURIOUS_INTERRUPT_VECTOR_REGISTER, 0x1ff );
    //HalLocalApicWrite( LAPIC_DIVIDE_CONFIG_REGISTER, 0x1 );

#if 1
    UCHAR Second = HalGetSecondFromCMOS( );

    HalLocalApicWrite( LAPIC_DIVIDE_CONFIG_REGISTER, 0x1 );

    while ( Second == HalGetSecondFromCMOS( ) )
        ;

    Second = HalGetSecondFromCMOS( );

    HalLocalApicWrite( LAPIC_INITIAL_COUNT_REGISTER, 0xFFFFFFFF );

    while ( Second == HalGetSecondFromCMOS( ) )
        ;

    HalLocalApicWrite( LAPIC_LVT_TIMER_REGISTER, LAPIC_MASKED );

    ULONG32 SecondTickCount = 0xFFFFFFFF - HalLocalApicRead( LAPIC_CURRENT_COUNT_REGISTER );
    SecondTickCount /= 100;

#else

    HalLocalApicWrite( LAPIC_INITIAL_COUNT_REGISTER, 0xFFFFFFFF );
    HalDelayExecutionPit( 10 );
    HalLocalApicWrite( LAPIC_LVT_TIMER_REGISTER, LAPIC_MASKED );
    ULONG32 SecondTickCount = 0xFFFFFFFF - HalLocalApicRead( LAPIC_CURRENT_COUNT_REGISTER );
#endif
    //0x20000 = periodic
    //624804
    HalLocalApicWrite( LAPIC_INITIAL_COUNT_REGISTER, SecondTickCount );
    HalLocalApicWrite( LAPIC_LVT_TIMER_REGISTER, 0x20 | TIMER_PERIODIC | LAPIC_MT_FIXED );
    HalLocalApicWrite( LAPIC_DIVIDE_CONFIG_REGISTER, 0x1 );

    //__cpuid( ( int* )IdRegisters, 0x15 );
    //HalLocalApicWrite( LAPIC_INITIAL_COUNT_REGISTER, ( IdRegisters[ 2 ] ) * 10000 );//( IdRegisters[ 2 ] * ( IdRegisters[ 1 ] / IdRegisters[ 0 ] ) ) / 10000 );

    //HalLocalApicWrite( LAPIC_LVT_TIMER_REGISTER, 0x20 | 0x20000 );
    //HalLocalApicWrite( LAPIC_DIVIDE_CONFIG_REGISTER, 0x3 );
    //__writecr2( ( ( IdRegisters[ 2 ] * ( IdRegisters[ 1 ] / IdRegisters[ 0 ] ) ) / 16 ) * 10000 );

}

VOID
HalApicRedirectIrq(
    _In_ ULONG           Irq,
    _In_ PKAPIC_REDIRECT Entry
)
{

    HalIoApicWrite( ( ULONG64 )HalIoApic, IO_APIC_REDIRECTION_TABLE( Irq ), Entry->Lower );
    HalIoApicWrite( ( ULONG64 )HalIoApic, IO_APIC_REDIRECTION_TABLE( Irq ) + 1, Entry->Upper );
}
