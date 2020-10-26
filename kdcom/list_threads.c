


#include "cmds.h"

//
//	internal ke header for PKTHREAD.
//

#include "../kernel/ke_struct.h" 


VOID
KdCmdListThreads(

)
{

	//
	//	should only be called when g_ProcessorBreak == TRUE.
	//

	ULONG32 CmdSize = sizeof( KD_CMDR_LIST_THREADS ) + ObjectTypeThread->TotalNumberOfObjects * sizeof( KD_THREAD );
	PKD_CMDR_LIST_THREADS ListThreads = ExAllocatePoolWithTag( CmdSize, TAGEX_CMD );
	KdInitCmdSz( ListThreads, KD_CMD_LIST_THREADS, CmdSize );

	ULONG32 i = 0;

	PLIST_ENTRY Flink = ObjectTypeThread->ObjectList.List;
	do {
		POBJECT_ENTRY_HEADER ObjectHeader = CONTAINING_RECORD( Flink, OBJECT_ENTRY_HEADER, ObjectList );
		PKTHREAD ThreadObject = ( PKTHREAD )( ObjectHeader + 1 );

		ListThreads->Thread[ i ].ProcessId = ThreadObject->Process->ActiveProcessId;
		ListThreads->Thread[ i ].ThreadId = ThreadObject->ActiveThreadId;
		i++;

		Flink = Flink->Flink;
	} while ( Flink != ObjectTypeThread->ObjectList.List );

	KdSendCmdSz( ListThreads, CmdSize );

	ExFreePoolWithTag( ListThreads, ' dmC' );
}
