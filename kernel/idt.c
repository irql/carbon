/*++

Module ObjectName:

	idt.c

Abstract:

	Implements interrupt descriptor procedures.

--*/


#include <carbsup.h>
#include "hal.h"
#include "acpi.h"
#include "ki.h"

VOID( *HalInterruptHandlers[ 256 ] )( PKTRAP_FRAME, PKPCR );

VOID
HalIdtInitialize(
	__inout PINTERRUPT_DESCRIPTOR_TABLE DescriptorTable,
	__inout PIDTR Idtr
)
{

	for ( USHORT i = 0; i < 256; i++ ) {

		DescriptorTable[ i ].Offset0 = ( USHORT )( HalInterruptHandlerTable[ i ] );
		DescriptorTable[ i ].Offset1 = ( USHORT )( HalInterruptHandlerTable[ i ] >> 16 );
		DescriptorTable[ i ].Offset2 = ( ULONG32 )( HalInterruptHandlerTable[ i ] >> 32 );
		DescriptorTable[ i ].InterruptStackTable = i == 0x80 ? 1 : ( i < 32 ? 2 : 3 );
		DescriptorTable[ i ].Zero = 0;
		DescriptorTable[ i ].CodeSelector = GDT_KERNEL_CODE64;
		DescriptorTable[ i ].TypeAttribute = 0x80 | ( i < 32 ? IDT_GATE_TYPE_TRAP32 : IDT_GATE_TYPE_INTERRUPT32 );
	}

	Idtr->Base = ( ULONG64 )DescriptorTable;
	Idtr->Limit = 256 * sizeof( INTERRUPT_DESCRIPTOR_TABLE ) - 1;
}

VOID
HalHandleInterrupt(
	__in PKTRAP_FRAME TrapFrame
)
{
	/*
		interruts 32 - 48 should not fire,
		the 8259a is mapped to these.
	*/

	/*
	if (TrapFrame->Interrupt >= (40) && TrapFrame->Interrupt < 48)
		__outbyte(PIC_SLAVE_IOPORT_COMMAND, PIC_EOI_COMMAND);
	__outbyte(PIC_MASTER_IOPORT_COMMAND, PIC_EOI_COMMAND);
	*/

	PKPCR Processor = ( PKPCR )__readmsr( MSR_GS_KERNEL_BASE );

	if ( Processor ) {

		HalLocalApicWrite( LOCAL_APIC_END_OF_INTERRUPT_REGISTER, 0 );
	}

	//DbgPrint("cpu%d - stack at: %#P\n", Processor->AcpiId, TrapFrame + 1);

	if ( HalInterruptHandlers[ TrapFrame->Interrupt ] )
		HalInterruptHandlers[ TrapFrame->Interrupt ]( TrapFrame, Processor );
}

VOID
HalIdtInstallHandler(
	__in UCHAR Number,
	__in VOID( *InterruptHandler )( PKTRAP_FRAME, PKPCR )
)
{
	HalInterruptHandlers[ Number ] = InterruptHandler;
}
