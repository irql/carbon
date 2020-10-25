

#pragma once

#include <windows.h>
#include "../kdcom/cmd.h"

VOID
KdCmdCrash(
	__in PKD_CMDR_CRASH Crash
);

VOID
KdCmdListThreads(
	__in PKD_CMDR_LIST_THREADS ListThreads
);

VOID
KdCmdListModules(
	__in PKD_CMDR_LIST_MODULES ListModules
);

VOID
KdCmdMessage(
	__in PKD_CMDR_MESSAGE Message
);

typedef VOID( *KD_CMD_HANDLER ) (
	__in PKD_BASE_COMMAND_RECIEVE
	);

extern KD_CMD_HANDLER g_CommandTable[ 0xFF ];

typedef WCHAR KD_CMD_STR[ 64 ];

extern KD_CMD_STR g_CommandStrings[ 0xFF ];

#define KD_DECLARE_HANDLER( handler )  (KD_CMD_HANDLER)(handler)
#define KD_DECLARE_NO_HANDLER KD_DECLARE_HANDLER( 0 )

#define KD_DECLARE_STRING( string ) (string)
#define KD_DECLARE_NO_STRING KD_DECLARE_STRING( L"" )

VOID
KdPrint(
	__in PWCHAR Buffer,
	__in ...
);