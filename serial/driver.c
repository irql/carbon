

#include "driver.h"

COM_PORT COM[ ] = { { 0x3F8, FALSE }, { 0x2F8, FALSE }, { 0x3E8, FALSE }, { 0x2E8, FALSE } };

NTSTATUS
DriverEntry(
	__in PDRIVER_OBJECT DriverObject
);

NTSTATUS
DriverUnload(
	__in PDRIVER_OBJECT DriverObject
);

NTSTATUS
DriverDispatch(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp
);

NTSTATUS
DriverEntry(
	__in PDRIVER_OBJECT DriverObject
)
{

	DriverObject->DriverUnload = DriverUnload;

	NTSTATUS ntStatus;
	UNICODE_STRING DiskLink = RTL_CONSTANT_UNICODE_STRING( L"\\COM" );
	PDEVICE_OBJECT DeviceObject;

	ntStatus = IoCreateDevice( DriverObject, &DiskLink, &DeviceObject );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	for ( ULONG32 i = 0; i < IRP_MJ_MAX; i++ ) {

		DeviceObject->MajorFunction[ i ] = DriverDispatch;
	}

	SerialInit( 0 );
	SerialInit( 1 );
	SerialInit( 2 );
	SerialInit( 3 );

	return STATUS_SUCCESS;
}

NTSTATUS
DriverUnload(
	__in PDRIVER_OBJECT DriverObject
)
{
	DriverObject;

	return STATUS_SUCCESS;
}

NTSTATUS
DriverDispatch(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp
)
{
	DeviceObject;

	Irp->UserIosb.Information = 0;
	Irp->UserIosb.Status = STATUS_INVALID_REQUEST;

	switch ( Irp->StackLocation->MajorFunction ) {
	case IRP_MJ_CREATE: {

		NTSTATUS ntStatus = STATUS_SUCCESS;

		if ( Irp->FileObject->FileName->Length != 1 ) {

			ntStatus = STATUS_INVALID_PATH;
			goto IrpCreateDone;
		}

		ULONG32 SerialPort = Irp->FileObject->FileName->Buffer[ 0 ] - '0';

		if ( SerialPort > 4 || SerialPort < 1 ) {

			ntStatus = STATUS_INVALID_PATH;
			goto IrpCreateDone;
		}

		SerialPort--;

		if ( COM[ SerialPort ].InUse ) {

			ntStatus = STATUS_IN_USE;
			goto IrpCreateDone;
		}

		Irp->FileObject->DiskId = SerialPort;

		COM[ SerialPort ].InUse = TRUE;

	IrpCreateDone:;
		Irp->UserIosb.Information = 0;
		Irp->UserIosb.Status = ntStatus;

		break;
	}
	case IRP_MJ_WRITE:

		SerialWrite( Irp->FileObject->DiskId, Irp->StackLocation->Parameters.Write.Length, Irp->SystemBuffer );
		Irp->UserIosb.Information = Irp->StackLocation->Parameters.Write.Length;
		Irp->UserIosb.Status = STATUS_SUCCESS;

		break;
	case IRP_MJ_READ:

		SerialRead( Irp->FileObject->DiskId, Irp->StackLocation->Parameters.Read.Length, Irp->SystemBuffer );
		Irp->UserIosb.Information = Irp->StackLocation->Parameters.Write.Length;
		Irp->UserIosb.Status = STATUS_SUCCESS;

		break;
	case IRP_MJ_CLOSE:

		COM[ Irp->FileObject->DiskId ].InUse = FALSE;
		Irp->UserIosb.Information = 0;
		Irp->UserIosb.Status = STATUS_SUCCESS;

		break;
	default:
		break;
	}

	return STATUS_SUCCESS;
}


