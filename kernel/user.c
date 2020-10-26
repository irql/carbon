


#include <carbsup.h>
#include "psp.h"
#include "ke.h"
#include "mm.h"

NTSTATUS
PsCreateUserProcess(
	__out PHANDLE ProcessHandle
)
{

	STATIC OBJECT_ATTRIBUTES DefaultAttributes = { 0, NULL };

	NTSTATUS ntStatus;
	PKPROCESS ProcessObject;

	ntStatus = ObpCreateObject( &ProcessObject, &DefaultAttributes, ObjectTypeProcess );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	ntStatus = ObCreateHandle( ProcessHandle, ProcessObject );

	if ( !NT_SUCCESS( ntStatus ) ) {

		ObDereferenceObject( ProcessObject );
	}


	PspInsertProcess( ProcessObject );
	ObDereferenceObject( ProcessObject );

	return STATUS_SUCCESS;
}

NTSTATUS
PsCreateUserThread(
	__out    PHANDLE         ThreadHandle,
	__in     HANDLE          ProcessHandle,
	__in     PKSTART_ROUTINE ThreadStart,
	__in     PVOID           ThreadContext,
	__in_opt ULONG32         UserStackSize,
	__in_opt ULONG32         KernelStackSize
)
{
	//NTSTATUS ntStatus;

	ThreadHandle;
	ProcessHandle;
	ThreadStart;
	ThreadContext;
	UserStackSize;
	KernelStackSize;

	return STATUS_SUCCESS;
}
