


#pragma once

#include <carbsup.h>
#include "kd.h"

VOID
KdCmdBreak(
	__in PKD_BASE_COMMAND_RECIEVE Cmd
);

VOID
KdCmdContinue(
	__in PKD_BASE_COMMAND_RECIEVE Cmd
);

VOID
KdCmdListThreads(
	__in PKD_BASE_COMMAND_RECIEVE Cmd
);

VOID
KdCmdListModules(
	__in PKD_BASE_COMMAND_RECIEVE Cmd
);

VOID
KdCmdThreadContext(
	__in PKD_CMDS_THREAD_CONTEXT Cmd
);