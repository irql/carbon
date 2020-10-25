/*++

Module ObjectName:

	driver.c

Abstract:

	I/O manager driver objects.

--*/


#include <carbsup.h>
#include "ob.h"
#include "io.h"
#include "ke.h"

NTSTATUS
IopCreateDriver(
	__out PDRIVER_OBJECT* DriverObject,
	__in PKMODULE DriverModule,
	__in PUNICODE_STRING DriverName
	)
{
	STATIC OBJECT_ATTRIBUTES DefaultAttributes = { OBJ_PERMANENT, NULL };
	NTSTATUS ntStatus;

	ntStatus = ObpCreateObject(DriverObject, &DefaultAttributes, ObjectTypeDriver);

	if (!NT_SUCCESS(ntStatus)) {

		return ntStatus;
	}

	_memcpy(&(*DriverObject)->DriverName, DriverName, sizeof(UNICODE_STRING));
	(*DriverObject)->DriverModule = DriverModule;
	ObReferenceObject(DriverModule);

	return STATUS_SUCCESS;
}

NTSTATUS
IoLoadDriver(
	__in PUNICODE_STRING FilePath
	)
{
	STATIC OBJECT_ATTRIBUTES DefaultAttributes = { 0, NULL };

	NTSTATUS ntStatus;
	HANDLE FileHandle;

	OBJECT_ATTRIBUTES ObjectAttributes = { 0, NULL };
	ObjectAttributes.ObjectName = FilePath;

	IO_STATUS_BLOCK Iosb;

	ntStatus = ZwCreateFile(&FileHandle, &Iosb, GENERIC_READ | GENERIC_WRITE, 0, &ObjectAttributes);

	if (!NT_SUCCESS(ntStatus)) {

		return ntStatus;
	}

	if (!NT_SUCCESS(Iosb.Status)) {

		return Iosb.Status;
	}

	PKMODULE Module;
	ntStatus = ObpCreateObject(&Module, &DefaultAttributes, ObjectTypeModule);

	if (!NT_SUCCESS(ntStatus)) {

		ZwClose(FileHandle);
		return ntStatus;
	}

	ntStatus = LdrSupLoadSupervisorModule(FileHandle, &Module->LoaderInfoBlock);

	if (!NT_SUCCESS(ntStatus)) {

		ObDereferenceObject(Module);
		ZwClose(FileHandle);
		return ntStatus;
	}

	ZwClose(FileHandle);

	_memcpy((void*)&Module->ImageName, FilePath, sizeof(UNICODE_STRING));

	PDRIVER_OBJECT Driver;
	IopCreateDriver(&Driver, Module, FilePath);

	((PKDRIVER_LOAD)Module->LoaderInfoBlock.ModuleEntry)(Driver);

	return STATUS_SUCCESS;
}
