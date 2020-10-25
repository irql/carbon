


#include <carbsup.h>
#include "kd.h"

VOID
KdReportCrash(
	__inout PKTRAP_FRAME TrapFrame,
	__in PKPCR Processor
)
{
	Processor;
	TrapFrame;

	KD_CMDR_CRASH CrashReport;
	KdInitCmd( &CrashReport, KD_CMD_CRASH );

	_memcpy( &CrashReport.R15, &TrapFrame->R15, sizeof( KTRAP_FRAME ) - 4096 );
	CrashReport.Cr0 = __readcr0( );
	CrashReport.Cr2 = __readcr2( );
	CrashReport.Cr3 = __readcr3( );
	CrashReport.Cr4 = __readcr4( );

	CrashReport.CpuIndex = Processor->CpuIndex;

	KdSendCmd( &CrashReport );

	KdDebugThread( );
}
