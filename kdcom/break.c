

#include "cmds.h"

VOLATILE BOOLEAN g_ProcessorBreak = FALSE;

VOID
KdProcessorBreak(
	__inout PKTRAP_FRAME TrapFrame,
	__in PKPCR Processor
)
{
	TrapFrame;
	Processor;

	KeEnterCriticalRegion( );

	while ( g_ProcessorBreak )
		;

	KeLeaveCriticalRegion( );

}

VOID
KdCmdBreak(

)
{
	if ( g_ProcessorBreak == TRUE ) {

		return;
	}

	KeEnterCriticalRegion( );

	HalIdtInstallHandler( 0x81, KdProcessorBreak );

	g_ProcessorBreak = TRUE;

	PKPCR CurrentProcessor = KeQueryCurrentProcessor( );

	for ( ULONG32 i = 0; i < KeQueryProcessorCount( ); i++ ) {

		if ( CurrentProcessor->CpuIndex == i )
			continue;

		PKPCR Processor;
		KeQueryLogicalProcessor( i, &Processor );

		HalLocalApicSendIpi( Processor->AcpiId, LOCAL_APIC_CR0_DEST_NORMAL | 0x81 );
	}

	KdCmdMessage( L"breakpoint." );
}
