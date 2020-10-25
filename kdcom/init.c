


#include <carbsup.h>
#include "kd.h"

UNICODE_STRING g_DefaultSerial = RTL_CONSTANT_UNICODE_STRING( L"\\COM1" );
HANDLE g_SerialHandle;
HANDLE g_ThreadHandle;

//
//	dlls are not properly implemented.
//

VOID
DllMain(
	__in ULONG64 Base,
	__in ULONG32 Reason
)
{
	Base;

	switch ( Reason ) {

	case REASON_DLL_LOAD:
		KdInitialize( );

		break;
	case REASON_DLL_UNLOAD:

		break;
	}

	return;
}

VOID
KdInitialize(

)
{
	NTSTATUS ntStatus;

	IO_STATUS_BLOCK Iosb;
	OBJECT_ATTRIBUTES ObjectAttributes = { 0 };

	ObjectAttributes.ObjectName = &g_DefaultSerial;
	ntStatus = ZwCreateFile( &g_SerialHandle, &Iosb, GENERIC_READ | GENERIC_WRITE, 0, &ObjectAttributes );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return;
	}

	if ( !NT_SUCCESS( Iosb.Status ) ) {

		return;
	}

	KD_BASE_COMMAND_SEND Send;

	Send.KdAckByte = KD_ACK_BYTE;
	Send.KdCommandByte = KD_CMD_CONNECT;

	ntStatus = ZwWriteFile( g_SerialHandle, &Iosb, &Send, sizeof( KD_BASE_COMMAND_SEND ), 0 );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return;
	}

	if ( !NT_SUCCESS( Iosb.Status ) ) {

		return;
	}

	HANDLE ProcessHandle = KeQueryCurrentProcess( );

	ntStatus = KeCreateThread( &g_ThreadHandle, ProcessHandle, ( PKSTART_ROUTINE )KdDebugThread, 0, 0, 0 );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return;
	}

	ZwClose( ProcessHandle );
}
