


#include <carbsup.h>
#include "kd.h"

VOID
KdDebugThread(

)
{

	IO_STATUS_BLOCK Iosb;

	UCHAR Buffer[ 512 ];
	PKD_BASE_COMMAND_RECIEVE RecvCmdBase = ( PKD_BASE_COMMAND_RECIEVE )&Buffer;
	

	while ( 1 ) {

		ZwReadFile( g_SerialHandle, &Iosb, RecvCmdBase, sizeof( KD_BASE_COMMAND_RECIEVE ), 0 );

		if ( RecvCmdBase->KdAckByte != KD_ACK_BYTE ) {

			continue;
		}

		if ( RecvCmdBase->KdCmdSize == sizeof( KD_BASE_COMMAND_RECIEVE ) ) {

			g_CommandTable[ RecvCmdBase->KdCommandByte ]( RecvCmdBase );
			continue;
		}

		ZwReadFile( g_SerialHandle, &Iosb, ( PCHAR )RecvCmdBase + sizeof( KD_BASE_COMMAND_RECIEVE ), RecvCmdBase->KdCmdSize, 0 );

		g_CommandTable[ RecvCmdBase->KdCommandByte ]( RecvCmdBase );
		

		//KeDelayExecutionThread( 100 );
	}
}
