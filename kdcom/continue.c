


#include "cmds.h"

EXTERN VOLATILE BOOLEAN g_ProcessorBreak;

VOID
KdCmdContinue(

)
{

	g_ProcessorBreak = FALSE;

	KeLeaveCriticalRegion( );
}