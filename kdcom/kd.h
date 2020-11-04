


#pragma once

#include "../kdcom/cmd.h"

#define KDSYSAPI DECLSPEC(DLLEXPORT)

typedef VOID( *KD_CMD_HANDLER ) (
	__in PKD_BASE_COMMAND_RECIEVE Cmd
	);

EXTERN KD_CMD_HANDLER g_CommandTable[ 0xFF ];

EXTERN HANDLE g_SerialHandle;
EXTERN HANDLE g_ThreadHandle;

VOID
KdInitialize(

);

VOID
KdDebugThread(

);

#define KdInitCmd( cmd, cmdbyte ) (cmd)->Base.KdAckByte = KD_ACK_BYTE; (cmd)->Base.KdCommandByte = cmdbyte; (cmd)->Base.KdCmdSize = sizeof( *cmd );
#define KdInitCmdSz( cmd, cmdbyte, size ) (cmd)->Base.KdAckByte = KD_ACK_BYTE; (cmd)->Base.KdCommandByte = cmdbyte; (cmd)->Base.KdCmdSize = (size);
#define KdSendCmd( cmd ) { IO_STATUS_BLOCK Iosb; ZwWriteFile( g_SerialHandle, &Iosb, (cmd), sizeof( *cmd ), 0 ); }
#define KdSendCmdSz( cmd, size ) { IO_STATUS_BLOCK Iosb; ZwWriteFile( g_SerialHandle, &Iosb, (cmd), (size), 0 ); }

KDSYSAPI
VOID
KdReportCrash(
	__inout PKTRAP_FRAME TrapFrame,
	__in PKPCR Processor
);

KDSYSAPI
VOID
KdCmdMessage(
	__in PWCHAR Message,
	__in ...
);