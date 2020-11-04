


#include "cmds.h"

//
//	internal ke header for PKTHREAD.
//

#include "../kernel/ki_struct.h" 
#include "../kernel/rtlp.h"


VOID
KdCmdThreadContext(
	__in PKD_CMDS_THREAD_CONTEXT Cmd
)
{

	KD_CMDR_THREAD_CONTEXT ThreadContext;
	KdInitCmd( &ThreadContext, KD_CMD_THREAD_CONTEXT );

	PLIST_ENTRY Flink = ObjectTypeThread->ObjectList.List;
	do {
		POBJECT_ENTRY_HEADER ObjectHeader = CONTAINING_RECORD( Flink, OBJECT_ENTRY_HEADER, ObjectList );
		PKTHREAD ThreadObject = ( PKTHREAD )( ObjectHeader + 1 );

		if ( ThreadObject->ActiveThreadId == Cmd->ThreadId ) {

			TRAPFRAME_TO_CONTEXT( &ThreadObject->ThreadControlBlock.Registers, ((PCONTEXT)(( PCHAR )&ThreadContext + sizeof( KD_BASE_COMMAND_RECIEVE ))) );

			break;
		}

		Flink = Flink->Flink;
	} while ( Flink != ObjectTypeThread->ObjectList.List );

	KdSendCmd( &ThreadContext );

}
