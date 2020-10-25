


#include "cmds.h"

#define KD_DECLARE_HANDLER( handler )  (KD_CMD_HANDLER)(handler)
#define KD_DECLARE_NONE KD_DECLARE_HANDLER( 0 )

KD_CMD_HANDLER g_CommandTable[ 0xFF ] = {
	KD_DECLARE_NONE,
	KD_DECLARE_HANDLER( KdCmdBreak ),
	KD_DECLARE_HANDLER( KdCmdContinue ),
	KD_DECLARE_NONE,
	KD_DECLARE_NONE,
	KD_DECLARE_HANDLER( KdCmdListThreads ),
	KD_DECLARE_HANDLER( KdCmdListModules )
};

