


#include "cmds.h"


VOID
KdCmdListModules(

)
{

	//
	//	should only be called when g_ProcessorBreak == TRUE.
	//

	ULONG32 CmdSize = sizeof( KD_CMDR_LIST_MODULES ) + ObjectTypeModule->TotalNumberOfObjects * sizeof( KD_MODULE );
	PKD_CMDR_LIST_MODULES ListModules = ExAllocatePoolWithTag( CmdSize, TAGEX_CMD );
	KdInitCmdSz( ListModules, KD_CMD_LIST_MODULES, CmdSize );

	ULONG32 i = 0;

	PLIST_ENTRY Flink = ObjectTypeModule->ObjectList.List;
	do {
		POBJECT_ENTRY_HEADER ObjectHeader = CONTAINING_RECORD( Flink, OBJECT_ENTRY_HEADER, ObjectList );
		PKMODULE ModuleObject = ( PKMODULE )( ObjectHeader + 1 );

		RtlStringCopy( ( PWCHAR )&ListModules->Module[ i ].ModuleName, ModuleObject->ImageName.Buffer );
		ListModules->Module[ i ].ModuleStart = ( ULONG64 )ModuleObject->LoaderInfoBlock.ModuleStart;
		ListModules->Module[ i ].ModuleEnd = ( ULONG64 )ModuleObject->LoaderInfoBlock.ModuleEnd;

		i++;

		Flink = Flink->Flink;
	} while ( Flink != ObjectTypeModule->ObjectList.List );

	KdSendCmdSz( ListModules, CmdSize );

	ExFreePoolWithTag( ListModules, ' dmC' );
}
