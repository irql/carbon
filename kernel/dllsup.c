


#include <carbsup.h>
#include "ldrsup.h"

NTSTATUS
LdrLoadDll(
	__in PUNICODE_STRING FilePath
)
{
	STATIC OBJECT_ATTRIBUTES DefaultAttributes = { 0, NULL };

	NTSTATUS ntStatus;
	HANDLE FileHandle;

	OBJECT_ATTRIBUTES ObjectAttributes = { 0, NULL };
	ObjectAttributes.ObjectName = FilePath;

	IO_STATUS_BLOCK Iosb;

	ntStatus = ZwCreateFile( &FileHandle, &Iosb, GENERIC_READ | GENERIC_WRITE, 0, &ObjectAttributes );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	if ( !NT_SUCCESS( Iosb.Status ) ) {

		return Iosb.Status;
	}

	PKMODULE Module;
	ntStatus = ObpCreateObject( &Module, &DefaultAttributes, ObjectTypeModule );

	if ( !NT_SUCCESS( ntStatus ) ) {

		ZwClose( FileHandle );
		return ntStatus;
	}

	ntStatus = LdrSupLoadSupervisorModule( FileHandle, &Module->LoaderInfoBlock );

	if ( !NT_SUCCESS( ntStatus ) ) {

		ObDereferenceObject( Module );
		ZwClose( FileHandle );
		return ntStatus;
	}

	ZwClose( FileHandle );

	_memcpy( ( void* )&Module->ImageName, FilePath, sizeof( UNICODE_STRING ) );

	( ( KDLL_ENTRY )Module->LoaderInfoBlock.ModuleEntry )( ( ULONG64 )Module->LoaderInfoBlock.ModuleStart, REASON_DLL_LOAD );

	return STATUS_SUCCESS;

}