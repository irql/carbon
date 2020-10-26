/*++

Module ObjectName:

	process.c

Abstract:

	Process management.

--*/

#include <carbsup.h>
#include "ke.h"
#include "ob.h"
#include "psp.h"

PKPROCESS KiSystemProcess = 0;

NTSTATUS
KiCreateProcess(
	__out PKPROCESS* NewProcessObject,
	__in PKMODULE AssociatedModule,
	__in PUNICODE_STRING ProcessName
)
{
	STATIC OBJECT_ATTRIBUTES DefaultAttributes = { 0, NULL };
	NTSTATUS ntStatus;

	PKPROCESS NewProcess;
	ntStatus = ObpCreateObject( ( PVOID* )&NewProcess, &DefaultAttributes, ObjectTypeProcess );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	if ( KiSystemProcess == 0 ) {

		KiSystemProcess = NewProcess;
		KeInitializeListHead( &KiSystemProcess->ActiveProcessLinks );
	}

	NewProcess->ActiveProcessId = KiGetUniqueIdentifier( );

	NewProcess->ProcessHandleTable.TotalNumberOfHandles = 0;
	NewProcess->ProcessHandleTable.HandleLinks.List = NULL;
	NewProcess->ProcessHandleTable.HandleLinks.Lock.ThreadLocked = NULL;

	HANDLE OwnHandle;
	ntStatus = ObpCreateHandle( &NewProcess->ProcessHandleTable, ( PVOID )NewProcess, &OwnHandle );

	if ( !NT_SUCCESS( ntStatus ) ) {

		//destroys the new process.
		ObDereferenceObject( NewProcess );
		return ntStatus;
	}

	NewProcess->ModuleObject = AssociatedModule;
	NewProcess->ProcessName = ProcessName;

	*NewProcessObject = NewProcess;

	return STATUS_SUCCESS;
}

PKPROCESS
KiQueryCurrentProcess(

)
{
	PKPROCESS Process = KeQueryCurrentProcessor( )->ThreadQueue->Process;
	ObReferenceObject( Process );

	return Process;
}

HANDLE
KeQueryCurrentProcess(

)
{
	HANDLE ProcessHandle;
	PKPROCESS Process = KeQueryCurrentProcessor( )->ThreadQueue->Process;

	ObpCreateHandle( ObpQueryCurrentHandleTable( ), ( PVOID )Process, &ProcessHandle );

	return ProcessHandle;
}

//should this like, create a process and a thread on the entry point of the module?
NTSTATUS
KeCreateProcess(
	__out PHANDLE NewProcessHandle,
	__in HANDLE AssociatedModule,
	__in PUNICODE_STRING ProcessName
)
{
	NTSTATUS ntStatus;

	PKMODULE AssociatedModuleObject;
	ntStatus = ObReferenceObjectByHandle( AssociatedModule, ( PVOID* )&AssociatedModuleObject );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	PKPROCESS NewProcessObject;
	ntStatus = KiCreateProcess( &NewProcessObject, AssociatedModuleObject, ProcessName );

	if ( !NT_SUCCESS( ntStatus ) ) {

		ObDereferenceObject( AssociatedModuleObject );
		return ntStatus;
	}

	ntStatus = ObpCreateHandle( ObpQueryCurrentHandleTable( ), ( PVOID )NewProcessObject, NewProcessHandle );

	if ( !NT_SUCCESS( ntStatus ) ) {

		ObDereferenceObject( NewProcessObject );
		ObDereferenceObject( AssociatedModuleObject );
		return ntStatus;
	}

	PspInsertProcess( NewProcessObject );

	ObDereferenceObject( NewProcessObject );
	ObDereferenceObject( AssociatedModuleObject );
	return ntStatus;
}