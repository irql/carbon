/*++

Module ObjectName:

	driver.c

Abstract:

	I/O manager driver objects.

--*/


#include <carbsup.h>
#include "obp.h"
#include "iop.h"
#include "ki.h"

NTSTATUS
IopCreateDriver(
	__out PDRIVER_OBJECT* DriverObject,
	__in PKMODULE DriverModule,
	__in PUNICODE_STRING DriverName
)
{
	STATIC OBJECT_ATTRIBUTES DefaultAttributes = { OBJ_PERMANENT, NULL };
	NTSTATUS ntStatus;

	ntStatus = ObpCreateObject( DriverObject, &DefaultAttributes, ObjectTypeDriver );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	_memcpy( &( *DriverObject )->DriverName, DriverName, sizeof( UNICODE_STRING ) );
	( *DriverObject )->DriverModule = DriverModule;
	ObReferenceObject( DriverModule );

	return STATUS_SUCCESS;
}

NTSTATUS
IoLoadDriver(
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

	PDRIVER_OBJECT Driver;
	IopCreateDriver( &Driver, Module, FilePath );

	( ( PKDRIVER_LOAD )Module->LoaderInfoBlock.ModuleEntry )( Driver );

	return STATUS_SUCCESS;
}
