/*++

Module ObjectName:

	irp.c

Abstract:

	Interrupt request packets.

--*/


#include <carbsup.h>
#include "ob.h"
#include "io.h"
#include "ke.h"

PIO_STACK_LOCATION
IoAllocateIrpStack(

	)
{


	return ExAllocatePoolWithTag(sizeof(IO_STACK_LOCATION), '  oI');
}

VOID
IoFreeIrpStack(
	__in PIO_STACK_LOCATION IrpStack
	)
{
	ExFreePoolWithTag(IrpStack, '  oI');

	return;
}

VOID
IoFreeIrp(
	__in PIRP Irp
)
{
	ExFreePoolWithTag(Irp, ' prI');

	return;
}

PIRP
IoAllocateIrp(

	)
{

	return ExAllocatePoolWithTag(sizeof(IRP), ' prI');
}

PIRP
IoBuildDeviceIoControlRequest(
	__in ULONG ControlCode,
	__in PVOID InputBuffer,
	__in ULONG InputBufferLength,
	__in PVOID OutputBuffer,
	__in ULONG OutputBufferLength
	)
{
	PIRP NewIrp = IoAllocateIrp();

	NewIrp->FileObject = NULL;
	
	NewIrp->StackLocation = IoAllocateIrpStack();
	NewIrp->RequestorMode = KernelMode;

	NewIrp->Thread = KiQueryCurrentThread();
	ObDereferenceObject(NewIrp->Thread);

	NewIrp->IssuingProcess = KiQueryCurrentProcess();
	ObDereferenceObject(NewIrp->IssuingProcess);

	NewIrp->StackLocation->MajorFunction = IRP_MJ_CONTROL;
	NewIrp->StackLocation->MinorFunction = 0;
	NewIrp->StackLocation->Parameters.Control.ControlCode = ControlCode;
	NewIrp->StackLocation->Parameters.Control.InputBufferLength = InputBufferLength;
	NewIrp->StackLocation->Parameters.Control.OutputBufferLength = OutputBufferLength;

	NewIrp->SystemBuffer = InputBuffer;
	NewIrp->UserBuffer = OutputBuffer;

	return NewIrp;
}


NTSTATUS
IoCallDevice(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp
	)
{

	if (DeviceObject->MajorFunction[Irp->StackLocation->MajorFunction] != NULL) {

		return DeviceObject->MajorFunction[Irp->StackLocation->MajorFunction](DeviceObject, Irp);
	}

	return STATUS_SUCCESS;
}
