/*++

Module ObjectName:

	iocom.c

Abstract:

	I/O manager communication.

--*/


#include <carbsup.h>
#include "obp.h"
#include "iop.h"
#include "ki.h"

NTSTATUS
IoInitializeIocb(
	__inout PIOCB Iocb
)
{
	NTSTATUS ntStatus;

	ntStatus = ObParseObjectDirectory(
		&Iocb->EntireName,
		&Iocb->ObjectName,
		&Iocb->RootName,
		&Iocb->DeviceObject );

	//DbgPrint("EntireName: %w, ObjectName: %w, RootName: %w, DeviceObject: %#.16P\n", Iocb->EntireName.Buffer, Iocb->ObjectName.Buffer, Iocb->RootName.Buffer, Iocb->DeviceObject);

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	return STATUS_SUCCESS;

}

NTSTATUS
ZwCreateFile(
	__out PHANDLE FileHandle,
	__out PIO_STATUS_BLOCK IoStatusBlock,
	__in ACCESS_MASK DesiredAccess,
	__in ULONG Disposition,
	__in POBJECT_ATTRIBUTES ObjectAttributes
)
{
	NTSTATUS ntStatus;
	PIOCB Iocb;
	STATIC OBJECT_ATTRIBUTES DefaultAttributes = { 0, NULL };

	ntStatus = ObpCreateObject( &Iocb, &DefaultAttributes, ObjectTypeIoCommunicationBlock );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	Iocb->InitialIrp = IoAllocateIrp( );
	_memcpy( &Iocb->EntireName, ObjectAttributes->ObjectName, sizeof( UNICODE_STRING ) );
	Iocb->EntireName.Buffer = ( PWCHAR )ExAllocatePoolWithTag( Iocb->EntireName.Size, TAGEX_STRING );
	_memcpy( Iocb->EntireName.Buffer, ObjectAttributes->ObjectName->Buffer, Iocb->EntireName.Size );

	ntStatus = IoInitializeIocb( Iocb );

	if ( !NT_SUCCESS( ntStatus ) ) {

		ObDereferenceObject( Iocb );
		return ntStatus;
	}

	Iocb->InitialIrp->RequestorMode = KernelMode;

	//
	//	we don't want references because this may mean if the process doesn't
	//	close the handle then the process object can never be freed.
	//

	Iocb->InitialIrp->Thread = KiQueryCurrentThread( );
	ObDereferenceObject( Iocb->InitialIrp->Thread );

	Iocb->InitialIrp->IssuingProcess = KiQueryCurrentProcess( );
	ObDereferenceObject( Iocb->InitialIrp->IssuingProcess );

	//
	//	this "object" will be the same for every irp sent from this process/handle
	//	we will only allocate it and set the RootDirectory field.
	//

	Iocb->InitialIrp->FileObject = IoAllocateFileObject( );
	Iocb->InitialIrp->FileObject->FileName = &Iocb->RootName;
	Iocb->InitialIrp->FileObject->DirectoryFile = IoAllocateFileObject( );

	//
	//	IRP_MJ_CREATE has these buffers set to NULL.
	//

	Iocb->InitialIrp->SystemBuffer = NULL;
	Iocb->InitialIrp->UserBuffer = NULL;

	Iocb->InitialIrp->StackLocation = IoAllocateIrpStack( );

	Iocb->InitialIrp->StackLocation->MajorFunction = IRP_MJ_CREATE;
	Iocb->InitialIrp->StackLocation->MinorFunction = 0;
	Iocb->InitialIrp->StackLocation->Parameters.Create.Access = DesiredAccess;
	Iocb->InitialIrp->StackLocation->Parameters.Create.Disposition = Disposition;

	ntStatus = IoCallDevice( Iocb->DeviceObject, Iocb->InitialIrp );

	if ( !NT_SUCCESS( ntStatus ) ) {

		ObDereferenceObject( Iocb );
		return ntStatus;
	}

	_memcpy( IoStatusBlock, &Iocb->InitialIrp->UserIosb, sizeof( IO_STATUS_BLOCK ) );

	if ( !NT_SUCCESS( Iocb->InitialIrp->UserIosb.Status ) ) {

		ObDereferenceObject( Iocb );
		return ntStatus;
	}

	ntStatus = ObCreateHandle( FileHandle, ( PVOID )Iocb );

	if ( !NT_SUCCESS( ntStatus ) ) {

		ObDereferenceObject( Iocb );
		return ntStatus;
	}

	return STATUS_SUCCESS;
}

NTSTATUS
ZwReadFile(
	__in HANDLE FileHandle,
	__out PIO_STATUS_BLOCK IoStatusBlock,
	__in PVOID Buffer,
	__in ULONG64 Length,
	__in ULONG64 ByteOffset
)
{
	NTSTATUS ntStatus;
	PIOCB Iocb;

	ntStatus = ObReferenceObjectByHandle( FileHandle, &Iocb );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	POBJECT_TYPE_DESCRIPTOR Type;
	ntStatus = ObQueryObjectType( Iocb, &Type );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	if ( Type != ObjectTypeIoCommunicationBlock ) {

		return STATUS_INVALID_HANDLE;
	}

	Iocb->InitialIrp->StackLocation->MajorFunction = IRP_MJ_READ;
	Iocb->InitialIrp->StackLocation->MinorFunction = 0;

	Iocb->InitialIrp->StackLocation->Parameters.Read.ByteOffset = ByteOffset;
	Iocb->InitialIrp->StackLocation->Parameters.Read.Length = Length;

	Iocb->InitialIrp->UserBuffer = NULL;
	Iocb->InitialIrp->SystemBuffer = Buffer;

	ntStatus = IoCallDevice( Iocb->DeviceObject, Iocb->InitialIrp );

	if ( !NT_SUCCESS( ntStatus ) ) {

		ObDereferenceObject( Iocb );
		return ntStatus;
	}

	_memcpy( IoStatusBlock, &Iocb->InitialIrp->UserIosb, sizeof( IO_STATUS_BLOCK ) );

	return STATUS_SUCCESS;
}

NTSTATUS
ZwWriteFile(
	__in HANDLE FileHandle,
	__out PIO_STATUS_BLOCK IoStatusBlock,
	__in PVOID Buffer,
	__in ULONG64 Length,
	__in ULONG64 ByteOffset
)
{
	NTSTATUS ntStatus;
	PIOCB Iocb;

	ntStatus = ObReferenceObjectByHandle( FileHandle, &Iocb );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	POBJECT_TYPE_DESCRIPTOR Type;
	ntStatus = ObQueryObjectType( Iocb, &Type );

	if ( !NT_SUCCESS( ntStatus ) ) {

		ObDereferenceObject( Iocb );
		return ntStatus;
	}

	if ( Type != ObjectTypeIoCommunicationBlock ) {

		ObDereferenceObject( Iocb );
		return STATUS_INVALID_HANDLE;
	}

	Iocb->InitialIrp->StackLocation->MajorFunction = IRP_MJ_WRITE;
	Iocb->InitialIrp->StackLocation->MinorFunction = 0;

	Iocb->InitialIrp->StackLocation->Parameters.Write.ByteOffset = ByteOffset;
	Iocb->InitialIrp->StackLocation->Parameters.Write.Length = Length;

	Iocb->InitialIrp->UserBuffer = NULL;
	Iocb->InitialIrp->SystemBuffer = Buffer;

	ntStatus = IoCallDevice( Iocb->DeviceObject, Iocb->InitialIrp );

	if ( !NT_SUCCESS( ntStatus ) ) {

		ObDereferenceObject( Iocb );
		return ntStatus;
	}

	_memcpy( IoStatusBlock, &Iocb->InitialIrp->UserIosb, sizeof( IO_STATUS_BLOCK ) );

	ObDereferenceObject( Iocb );
	return STATUS_SUCCESS;
}

NTSTATUS
ZwClose(
	__in HANDLE Handle
)
{
	NTSTATUS ntStatus;
	PIOCB Iocb;

	ntStatus = ObReferenceObjectByHandle( Handle, &Iocb );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ObCloseHandle( Handle );
	}

	POBJECT_TYPE_DESCRIPTOR Type;
	ntStatus = ObQueryObjectType( Iocb, &Type );

	if ( !NT_SUCCESS( ntStatus ) ) {

		ObDereferenceObject( Iocb );
		return ObCloseHandle( Handle );
	}

	if ( Type != ObjectTypeIoCommunicationBlock ) {

		ObDereferenceObject( Iocb );
		return ObCloseHandle( Handle );
	}

	Iocb->InitialIrp->StackLocation->MajorFunction = IRP_MJ_CLOSE;
	Iocb->InitialIrp->StackLocation->MinorFunction = 0;

	Iocb->InitialIrp->UserBuffer = NULL;
	Iocb->InitialIrp->SystemBuffer = NULL;

	ntStatus = IoCallDevice( Iocb->DeviceObject, Iocb->InitialIrp );

	ExFreePoolWithTag( Iocb->EntireName.Buffer, ' rtS' );

	IoFreeFileObject( Iocb->InitialIrp->FileObject );
	IoFreeIrpStack( Iocb->InitialIrp->StackLocation );
	IoFreeIrp( Iocb->InitialIrp );

	ObDereferenceObject( Iocb );
	return ObCloseHandle( Handle );
}

NTSTATUS
ZwQueryDirectoryFile(
	__in HANDLE FileHandle,
	__out PIO_STATUS_BLOCK IoStatusBlock,
	__in PVOID FileInformation,
	__in ULONG Length,
	__in FILE_INFORMATION_CLASS FileInformationClass,
	__in_opt PUNICODE_STRING FileName
)
{
	NTSTATUS ntStatus;
	PIOCB Iocb;

	ntStatus = ObReferenceObjectByHandle( FileHandle, &Iocb );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	POBJECT_TYPE_DESCRIPTOR Type;
	ntStatus = ObQueryObjectType( Iocb, &Type );

	if ( !NT_SUCCESS( ntStatus ) ) {

		ObDereferenceObject( Iocb );
		return ntStatus;
	}

	if ( Type != ObjectTypeIoCommunicationBlock ) {

		ObDereferenceObject( Iocb );
		return STATUS_INVALID_HANDLE;
	}

	Iocb->InitialIrp->StackLocation->MajorFunction = IRP_MJ_QUERY_DIRECTORY_FILE;
	Iocb->InitialIrp->StackLocation->MinorFunction = 0;

	Iocb->InitialIrp->StackLocation->Parameters.QueryDirectory.FileInformationClass = FileInformationClass;
	Iocb->InitialIrp->StackLocation->Parameters.QueryDirectory.Length = Length;
	Iocb->InitialIrp->StackLocation->Parameters.QueryDirectory.FileName = FileName;

	Iocb->InitialIrp->UserBuffer = NULL;
	Iocb->InitialIrp->SystemBuffer = FileInformation;

	ntStatus = IoCallDevice( Iocb->DeviceObject, Iocb->InitialIrp );

	if ( !NT_SUCCESS( ntStatus ) ) {

		ObDereferenceObject( Iocb );
		return ntStatus;
	}

	_memcpy( IoStatusBlock, &Iocb->InitialIrp->UserIosb, sizeof( IO_STATUS_BLOCK ) );

	ObDereferenceObject( Iocb );
	return STATUS_SUCCESS;
}

NTSTATUS
ZwQueryInformationFile(
	__in HANDLE FileHandle,
	__out PIO_STATUS_BLOCK IoStatusBlock,
	__in PVOID FileInformation,
	__in ULONG Length,
	__in FILE_INFORMATION_CLASS FileInformationClass
)
{
	NTSTATUS ntStatus;
	PIOCB Iocb;

	ntStatus = ObReferenceObjectByHandle( FileHandle, &Iocb );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	POBJECT_TYPE_DESCRIPTOR Type;
	ntStatus = ObQueryObjectType( Iocb, &Type );

	if ( !NT_SUCCESS( ntStatus ) ) {

		ObDereferenceObject( Iocb );
		return ntStatus;
	}

	if ( Type != ObjectTypeIoCommunicationBlock ) {

		ObDereferenceObject( Iocb );
		return STATUS_INVALID_HANDLE;
	}

	Iocb->InitialIrp->StackLocation->MajorFunction = IRP_MJ_QUERY_INFORMATION_FILE;
	Iocb->InitialIrp->StackLocation->MinorFunction = 0;

	Iocb->InitialIrp->StackLocation->Parameters.QueryFile.FileInformationClass = FileInformationClass;
	Iocb->InitialIrp->StackLocation->Parameters.QueryFile.Length = Length;

	Iocb->InitialIrp->UserBuffer = NULL;
	Iocb->InitialIrp->SystemBuffer = FileInformation;

	ntStatus = IoCallDevice( Iocb->DeviceObject, Iocb->InitialIrp );

	if ( !NT_SUCCESS( ntStatus ) ) {

		ObDereferenceObject( Iocb );
		return ntStatus;
	}

	_memcpy( IoStatusBlock, &Iocb->InitialIrp->UserIosb, sizeof( IO_STATUS_BLOCK ) );

	ObDereferenceObject( Iocb );
	return STATUS_SUCCESS;
}

NTSTATUS
ZwSetInformationFile(
	__in HANDLE FileHandle,
	__out PIO_STATUS_BLOCK IoStatusBlock,
	__in PVOID FileInformation,
	__in ULONG Length,
	__in FILE_INFORMATION_CLASS FileInformationClass
)
{
	NTSTATUS ntStatus;
	PIOCB Iocb;

	ntStatus = ObReferenceObjectByHandle( FileHandle, &Iocb );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	POBJECT_TYPE_DESCRIPTOR Type;
	ntStatus = ObQueryObjectType( Iocb, &Type );

	if ( !NT_SUCCESS( ntStatus ) ) {

		ObDereferenceObject( Iocb );
		return ntStatus;
	}

	if ( Type != ObjectTypeIoCommunicationBlock ) {

		ObDereferenceObject( Iocb );
		return STATUS_INVALID_HANDLE;
	}

	Iocb->InitialIrp->StackLocation->MajorFunction = IRP_MJ_SET_INFORMATION_FILE;
	Iocb->InitialIrp->StackLocation->MinorFunction = 0;

	Iocb->InitialIrp->StackLocation->Parameters.SetFile.FileInformationClass = FileInformationClass;
	Iocb->InitialIrp->StackLocation->Parameters.SetFile.Length = Length;

	Iocb->InitialIrp->UserBuffer = NULL;
	Iocb->InitialIrp->SystemBuffer = FileInformation;

	ntStatus = IoCallDevice( Iocb->DeviceObject, Iocb->InitialIrp );

	if ( !NT_SUCCESS( ntStatus ) ) {

		ObDereferenceObject( Iocb );
		return ntStatus;
	}

	_memcpy( IoStatusBlock, &Iocb->InitialIrp->UserIosb, sizeof( IO_STATUS_BLOCK ) );

	ObDereferenceObject( Iocb );
	return STATUS_SUCCESS;
}

NTSTATUS
ZwDeviceIoControlFile(
	__in HANDLE FileHandle,
	__out PIO_STATUS_BLOCK IoStatusBlock,
	__in ULONG IoControlCode,
	__in PVOID InputBuffer,
	__in ULONG InputBufferLength,
	__in PVOID OutputBuffer,
	__in ULONG OutputBufferLength
)
{
	NTSTATUS ntStatus;
	PIOCB Iocb;

	ntStatus = ObReferenceObjectByHandle( FileHandle, &Iocb );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	POBJECT_TYPE_DESCRIPTOR Type;
	ntStatus = ObQueryObjectType( Iocb, &Type );

	if ( !NT_SUCCESS( ntStatus ) ) {

		ObDereferenceObject( Iocb );
		return ntStatus;
	}

	if ( Type != ObjectTypeIoCommunicationBlock ) {

		ObDereferenceObject( Iocb );
		return STATUS_INVALID_HANDLE;
	}

	Iocb->InitialIrp->StackLocation->MajorFunction = IRP_MJ_CONTROL;
	Iocb->InitialIrp->StackLocation->MinorFunction = 0;

	Iocb->InitialIrp->StackLocation->Parameters.Control.ControlCode = IoControlCode;
	Iocb->InitialIrp->StackLocation->Parameters.Control.InputBufferLength = InputBufferLength;
	Iocb->InitialIrp->StackLocation->Parameters.Control.OutputBufferLength = OutputBufferLength;

	Iocb->InitialIrp->UserBuffer = OutputBuffer;
	Iocb->InitialIrp->SystemBuffer = InputBuffer;

	ntStatus = IoCallDevice( Iocb->DeviceObject, Iocb->InitialIrp );

	if ( !NT_SUCCESS( ntStatus ) ) {

		ObDereferenceObject( Iocb );
		return ntStatus;
	}

	_memcpy( IoStatusBlock, &Iocb->InitialIrp->UserIosb, sizeof( IO_STATUS_BLOCK ) );

	ObDereferenceObject( Iocb );
	return STATUS_SUCCESS;
}