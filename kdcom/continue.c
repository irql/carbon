


#include "cmds.h"

EXTERN VOLATILE BOOLEAN g_ProcessorBreak;

VOID
KdCmdContinue(
	__in PKD_BASE_COMMAND_RECIEVE Cmd
)
{
	Cmd;

	g_ProcessorBreak = FALSE;

	KeLeaveCriticalRegion( );
}