/*++

Module ObjectName:

	apic.c

Abstract:

	!

--*/

#include <carbsup.h>
#include "hal.h"
#include "acpi.h"
#include "mm.h"

ULONG64 MadtLocalApic;
ULONG64 MadtIoApic0;

VOID
HalApicSetLocalBaseMsr(
	__in ULONG64 ApicBase
)
{

	__writemsr( MSR_APIC_BASE, ApicBase );
}

ULONG64
HalApicGetLocalBaseMsr(

)
{

	return __readmsr( MSR_APIC_BASE );
}

ULONG32
HalIoApicRead(
	__in ULONG64 IoApicAddress,
	__in ULONG32 Register
)
{
	ULONG32* IoApic = ( ULONG32* )IoApicAddress;
	IoApic[ 0 ] = ( Register & 0xff );

	return IoApic[ 4 ];
}

VOID
HalIoApicWrite(
	__in ULONG64 IoApicAddress,
	__in ULONG32 Register,
	__in ULONG32 Value
)
{
	ULONG32* IoApic = ( ULONG32* )IoApicAddress;
	IoApic[ 0 ] = ( Register & 0xff );
	IoApic[ 4 ] = Value;

	return;
}

ULONG32
HalLocalApicRead(
	__in ULONG32 Register
)
{
	//DbgPrint("LAPIC Read: %.16P\n", (ULONG32*)((ULONG64)Madt->LocalApicAddress + Register));

	return *( ULONG32* )( ( ULONG64 )MadtLocalApic + Register );
}

VOID
HalLocalApicWrite(
	__in ULONG32 Register,
	__in ULONG32 Value
)
{
	//DbgPrint("LAPIC Write: %.16P\n", (ULONG32*)((ULONG64)Madt->LocalApicAddress + Register));

	*( ULONG32* )( ( ULONG64 )MadtLocalApic + Register ) = Value;
}

VOID
HalLocalApicEnable(

)
{

	//MmAllocateMemoryAt( ( ULONG64 )Madt->LocalApicAddress, ( ULONG64 )Madt->LocalApicAddress, 0x1000, PAGE_READ | PAGE_WRITE );
	MadtLocalApic = ( ULONG64 )MmAllocateMemoryAtPhysical( Madt->LocalApicAddress, 0x1000, PAGE_READ | PAGE_WRITE );

	HalLocalApicWrite( LOCAL_APIC_LVT_TIMER_REGISTER, LOCAL_APIC_CR0_DEST_DISABLE );
	//HalLocalApicWrite( LOCAL_APIC_LVT_PERFORMANCE_MONITORING_COUNTERS_REGISTER, LOCAL_APIC_CR0_DEST_NMI );
	HalLocalApicWrite( LOCAL_APIC_LVT_LINT0_REGISTER, LOCAL_APIC_CR0_DEST_DISABLE );
	HalLocalApicWrite( LOCAL_APIC_LVT_LINT1_REGISTER, LOCAL_APIC_CR0_DEST_DISABLE );
	HalLocalApicWrite( LOCAL_APIC_TASK_PRIORITY_REGISTER, 0 );

	__writemsr( MSR_APIC_BASE, __readmsr( MSR_APIC_BASE ) | ( 1 << 11 ) );

	//HalLocalApicRead( LOCAL_APIC_SPURIOUS_INTERRUPT_VECTOR_REGISTER ) |

	HalLocalApicWrite( LOCAL_APIC_SPURIOUS_INTERRUPT_VECTOR_REGISTER, 0x1ff );

	//MmAllocateMemoryAt( ( ULONG64 )MadtIoApics[ 0 ]->IoApicAddress, ( ULONG64 )MadtIoApics[ 0 ]->IoApicAddress, 0x2000, PAGE_READ | PAGE_WRITE );
	MadtIoApic0 = ( ULONG64 )MmAllocateMemoryAtPhysical( MadtIoApics[ 0 ]->IoApicAddress, 0x2000, PAGE_READ | PAGE_WRITE );
}

VOID
HalLocalApicSendIpi(
	__in ULONG32 CpuApciId,
	__in ULONG32 Cr0Flags
)
{
	HalLocalApicWrite( LOCAL_APIC_INTERRUPT_COMMAND_REGISTER( 1 ), CpuApciId << 24 );
	HalLocalApicWrite( LOCAL_APIC_INTERRUPT_COMMAND_REGISTER( 0 ), Cr0Flags );
}

UCHAR
HalGetSecondFromCMOS(

)
{
	__outbyte( 0x70, 0 );
	return __inbyte( 0x71 );
}

VOID
HalLocalApicTimerEnable(

)
{
#if 0
	HalLocalApicWrite( LOCAL_APIC_DIVIDE_CONFIG_REGISTER, 0x3 );
	HalLocalApicWrite( LOCAL_APIC_INITIAL_COUNT_REGISTER, 0xFFFFFFFF );

	HalPitDelay( 10 );

	ULONG32 MillisecondTickCount = 0xFFFFFFFF - HalLocalApicRead( LOCAL_APIC_CURRENT_COUNT_REGISTER );

	//0x20000 = periodic
	HalLocalApicWrite( LOCAL_APIC_DIVIDE_CONFIG_REGISTER, 0x3 );
	HalLocalApicWrite( LOCAL_APIC_INITIAL_COUNT_REGISTER, MillisecondTickCount );
	HalLocalApicWrite( LOCAL_APIC_LVT_TIMER_REGISTER, 0x80 | 0x20000 );

	//DbgPrint( "Apic timer count: %d\n", MillisecondTickCount );
#else

	UCHAR Second = HalGetSecondFromCMOS( );

	HalLocalApicWrite( LOCAL_APIC_DIVIDE_CONFIG_REGISTER, 0x3 );

	while ( Second == HalGetSecondFromCMOS( ) )
		;

	Second = HalGetSecondFromCMOS( );

	HalLocalApicWrite( LOCAL_APIC_INITIAL_COUNT_REGISTER, 0xFFFFFFFF );

	while ( Second == HalGetSecondFromCMOS( ) )
		;

	HalLocalApicWrite( LOCAL_APIC_LVT_TIMER_REGISTER, LOCAL_APIC_CR0_DEST_DISABLE );

	ULONG32 SecondTickCount = 0xFFFFFFFF - HalLocalApicRead( LOCAL_APIC_CURRENT_COUNT_REGISTER );
	SecondTickCount /= 100;

	//0x20000 = periodic
	HalLocalApicWrite( LOCAL_APIC_INITIAL_COUNT_REGISTER, SecondTickCount );
	HalLocalApicWrite( LOCAL_APIC_LVT_TIMER_REGISTER, 0x80 | 0x20000 );
	HalLocalApicWrite( LOCAL_APIC_DIVIDE_CONFIG_REGISTER, 0x3 );
#endif
	return;
}

VOID
HalIoApicRedirectIrq(
	__in UCHAR Irq,
	__in PREDIRECTION_ENTRY RedirectionEntry
)
{
	//should always be an io-apic otherwise you can FUCK OFF.


	HalIoApicWrite( MadtIoApic0, IO_APIC_REDIRECTION_TABLE( Irq ), RedirectionEntry->Lower );
	HalIoApicWrite( MadtIoApic0, IO_APIC_REDIRECTION_TABLE( Irq ) + 1, RedirectionEntry->Upper );
}