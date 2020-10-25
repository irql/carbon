


#include <carbsup.h>
#include "kd.h"

VOID
KdDebugThread(

)
{

	IO_STATUS_BLOCK Iosb;
	KD_BASE_COMMAND_SEND Recieved;

	while ( 1 ) {

		ZwReadFile( g_SerialHandle, &Iosb, &Recieved, sizeof( KD_BASE_COMMAND_SEND ), 0 );

		if ( Recieved.KdAckByte != KD_ACK_BYTE ) {

			continue;
		}

		g_CommandTable[ Recieved.KdCommandByte ]( );

		//KeDelayExecutionThread( 100 );
	}
}
