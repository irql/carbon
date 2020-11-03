


#include "cmds.h"


VOID
KdCmdMessage(
	__in PWCHAR Message,
	__in ...
)
{
	va_list Args;
	va_start( Args, Message );

	WCHAR Buffer[ 512 ];
	vsprintfW( Buffer, Message, Args );

	va_end( Args );

	ULONG32 MessageSize = sizeof( KD_CMDR_MESSAGE ) + ( ( lstrlenW( Buffer ) + 1 ) * sizeof( WCHAR ) );
	PKD_CMDR_MESSAGE CmdMessage = ExAllocatePoolWithTag( MessageSize, TAGEX_MESSAGE );
	KdInitCmdSz( CmdMessage, KD_CMD_MESSAGE, MessageSize );

	lstrcpyW( CmdMessage->Message, Buffer );

	KdSendCmdSz( CmdMessage, MessageSize );
	ExFreePoolWithTag( CmdMessage, ' gsM' );
}