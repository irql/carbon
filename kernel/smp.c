/*++

Module ObjectName:

	smp.c

Abstract:

	Symmetric multi-processing.

--*/

#include <carbsup.h>
#include "hal.h"
#include "ke.h"
#include "mm.h"
#include "acpi.h"

ULONG64 HalXsave = 0;
ULONG64 HalSimdSaveRegion = 512;

VOLATILE ULONG32 KiCurrentCpuInitializing = 0;

/*
	Every processor should get the same GDT & IDT
*/

VOID
HalSmpInitializeCpu0(

)
/*++

Routine Description:

	Initializes the BSP.
	- Sets up the GDT properly via HalGdt* API's
	- Sets up the IDT properly via HalIdt* API's
	- Sets up a KPCR
	- Cum.

Arguments:

	None.

Return Value:

	None.

--*/
{
	NTSTATUS ntStatus;

	PKPCR Processor;
	ntStatus = KiCreateKpcr( MadtProcessorLocalApics[ 0 ]->ApicId, &Processor );

	if ( !NT_SUCCESS( ntStatus ) ) {

		//hm, we cant continue, bugcheck?
		__halt( );
	}

	HalIdtInitialize(
		( PINTERRUPT_DESCRIPTOR_TABLE )
		MmAllocateMemory( sizeof( INTERRUPT_DESCRIPTOR_TABLE[ 256 ] ), PAGE_READ | PAGE_WRITE ),
		&Processor->Idtr );

	for ( UCHAR i = 0; i < 32; i++ )
		HalIdtInstallHandler( i, KiBugCheckTrap );
	//HalIdtInstallHandler(14, MmpPageFaultTrap);
	HalIdtInstallHandler( 0x80, KiThreadDispatcher );

	__lidt( &Processor->Idtr );
	HalSetInterruptFlag( );

	/*
		The code64 descriptor should have a limit of 0xf0000 (gr set)

		this is because if this was used for an AP and it set the GDT in protected mode
		then the cpu should check the limit of the jump and then switch the cs.

		OK, so i want the ist1 to contain a stack for the scheduler only.

		the scheduler will use 0x80

		ist2 is for ISR's

		these will never be used by aps in their trampoline so its same to have no gr & limit
	*/

	HalGdtCreate( &Processor->Gdtr );

	GLOBAL_DESCRIPTOR_TABLE_ENTRY Code64 = { 0, 0, 0, {0, 1, 0, 1, 1, 0, 1}, {0xf, 0, 1, 0, 1}, 0 };
	GLOBAL_DESCRIPTOR_TABLE_ENTRY Data = { 0, 0, 0, {0, 1, 0, 0, 1, 0, 1}, {0, 0, 0, 0, 0}, 0 };

	HalGdtAddEntry( &Processor->Gdtr, &Code64 );
	HalGdtAddEntry( &Processor->Gdtr, &Data );

	PTSS TaskStateSegment = ( PTSS )ExAllocatePoolWithTag( sizeof( TSS ), ' SST' );
	_memset( ( void* )TaskStateSegment, 0, sizeof( TSS ) );

	TaskStateSegment->Rsp0 = 0;
	TaskStateSegment->Ist1 = ( ULONG64 )MmAllocateMemory( KERNEL_STACK_SIZE, PAGE_READ | PAGE_WRITE ) + KERNEL_STACK_SIZE;
	TaskStateSegment->Ist2 = ( ULONG64 )MmAllocateMemory( KERNEL_STACK_SIZE, PAGE_READ | PAGE_WRITE ) + KERNEL_STACK_SIZE;
	TaskStateSegment->IopbOffset = sizeof( TSS );

	Processor->TaskStateDescriptor = HalGdtAddTss( &Processor->Gdtr, TaskStateSegment, sizeof( TSS ) );

	_lgdt( &Processor->Gdtr );
	__ltr( Processor->TaskStateDescriptor );

	__writemsr( MSR_GS_BASE, ( ULONG64 )Processor );
	__writemsr( MSR_GS_KERNEL_BASE, ( ULONG64 )Processor );

}

EXTERN INTERRUPT_DESCRIPTOR_TABLE BspBootDescriptor[ 256 ];

VOID
HalSmpInitializeCpu(

)
/*++

Routine Description:

	This procedure is called by AP's immediately after executing the trampoline.
	The AP's jump here, so there is no return address, they should loop and halt until the dispatcher
	wants them.

Arguments:

	None.

Return Value:

	Bitchass.

--*/
{

	ULONG32 AcpiId = KiCurrentCpuInitializing;

	//DbgPrint(L"cpu%d, started.\n", AcpiId);

	IDTR BspIdtr;
	BspIdtr.Base = ( ULONG64 )BspBootDescriptor;
	BspIdtr.Limit = 256 * sizeof( INTERRUPT_DESCRIPTOR_TABLE ) - 1;
	__lidt( &BspIdtr );

	HalSmpEnableCpuFeatures( );

	NTSTATUS ntStatus;

	PKPCR Processor;
	ntStatus = KiCreateKpcr( AcpiId, &Processor );

	if ( !NT_SUCCESS( ntStatus ) ) {

		//bugcheck?
		__halt( );
	}

	PKPCR Processor0;
	ntStatus = KeQueryLogicalProcessor( 0, &Processor0 );

	if ( !NT_SUCCESS( ntStatus ) ) {

		//bugcheck.
		__halt( );
	}

	_memcpy( ( void* )&Processor->Idtr, ( void* )&Processor0->Idtr, sizeof( IDTR ) );
	_memcpy( ( void* )&Processor->Gdtr, ( void* )&Processor0->Gdtr, sizeof( GDTR ) );

	__lidt( &Processor->Idtr );

	PTSS TaskStateSegment = ( PTSS )ExAllocatePoolWithTag( sizeof( TSS ), ' SST' );
	_memset( ( void* )TaskStateSegment, 0, sizeof( TSS ) );

	TaskStateSegment->Rsp0 = 0;
	TaskStateSegment->Ist1 = ( ULONG64 )MmAllocateMemory( KERNEL_STACK_SIZE, PAGE_READ | PAGE_WRITE ) + KERNEL_STACK_SIZE;
	TaskStateSegment->Ist2 = ( ULONG64 )MmAllocateMemory( KERNEL_STACK_SIZE, PAGE_READ | PAGE_WRITE ) + KERNEL_STACK_SIZE;
	TaskStateSegment->IopbOffset = sizeof( TSS );

	Processor->TaskStateDescriptor = HalGdtAddTss( &Processor->Gdtr, TaskStateSegment, sizeof( TSS ) );

	_lgdt( &Processor->Gdtr );
	__ltr( Processor->TaskStateDescriptor );

	__writemsr( MSR_GS_BASE, ( ULONG64 )Processor );
	__writemsr( MSR_GS_KERNEL_BASE, ( ULONG64 )Processor );

	HalLocalApicEnable( );

	HalLocalApicTimerEnable( );

	HalSetInterruptFlag( );

	KiCurrentCpuInitializing = 0;

	while ( 1 )
		__halt( );
}

VOID
printf(
	__in CHAR* Format,
	__in ...
);

VOID
HalSmpEnableCpuFeatures(

)
{

	__writecr0( ( ( __readcr0( ) | ( 1 << 1 ) ) & ~( 1 << 2 ) ) & ~0x60000000i64 );
	__writecr4( __readcr4( ) | ( 3 << 9 ) );


	//https://forum.osdev.org/viewtopic.php?f=1&t=29034
	//__writemsr( MSR_PAT, ( __readmsr( MSR_PAT ) & 0xFFFFFFFF ) | ( ( ( ULONG64 )( ( PAT_WRITECOMBINING << 4 ) | PAT_WRITEPROTECTED ) ) << 32 ) );

	int r[ 4 ];

	__cpuid( r, 1 );
#if 0
	//xsave
	if ( r[ 2 ] & ( 1 << 26 ) ) {
		__writecr4( __readcr4( ) | ( 1 << 18 ) );

		ULONG64 xcr0 = ( 1 << 0 ) | ( 1 << 1 );

		//(1 << 16) is for avx512.
		//AVX
		if ( r[ 2 ] & ( 1 << 28 ) )
			xcr0 |= ( 1 << 2 );

		__cpuid( r, 13 );
		HalSimdSaveRegion = r[ 2 ];

		_xsetbv( 0, xcr0 );
		HalXsave = 1;

		printf( "pogs %p\n", HalSimdSaveRegion );
		__halt( );

	}
#endif

	__fninit( );
}