


#include <carbsup.h>
#include "ldrpsup.h"

NTSTATUS
LdrLoadDll(
	__in PUNICODE_STRING FilePath
)
{
	STATIC OBJECT_ATTRIBUTES DefaultAttributes = { 0, NULL };

	NTSTATUS ntStatus;

	PKMODULE Module;
	ntStatus = ObpCreateObject( &Module, &DefaultAttributes, ObjectTypeModule );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	ntStatus = LdrSupLoadSupervisorModule( FilePath, &Module->LoaderInfoBlock );

	if ( !NT_SUCCESS( ntStatus ) ) {

		ObDereferenceObject( Module );
		return ntStatus;
	}

	_memcpy( ( void* )&Module->ImageName, FilePath, sizeof( UNICODE_STRING ) );

	( ( KDLL_ENTRY )Module->LoaderInfoBlock.ModuleEntry )( ( ULONG64 )Module->LoaderInfoBlock.ModuleStart, REASON_DLL_LOAD );

	return STATUS_SUCCESS;

}