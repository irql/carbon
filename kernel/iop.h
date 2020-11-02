/*++

Module ObjectName:

	io.h

Abstract:

	I/O manager.

--*/

#pragma once


typedef struct _IOCB {
	PDEVICE_OBJECT DeviceObject;
	UNICODE_STRING EntireName;
	UNICODE_STRING RootName;
	UNICODE_STRING ObjectName;
	PIRP InitialIrp;
} IOCB, *PIOCB;

NTSTATUS
IopCreateDriver(
	__out PDRIVER_OBJECT* DriverObject,
	__in PKMODULE DriverModule,
	__in PUNICODE_STRING DriverName
	);

PIRP
IoAllocateIrp(

	);

VOID
IoFreeIrp(
	__in PIRP Irp
	);

PIO_STACK_LOCATION
IoAllocateIrpStack(

	);

VOID
IoFreeIrpStack(
	__in PIO_STACK_LOCATION IrpStack
	);

PFILE_OBJECT
IoAllocateFileObject(

	);

VOID
IoFreeFileObject(
	__in PFILE_OBJECT FileObject
	);



