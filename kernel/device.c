/*++

Module ObjectName:

	device.c

Abstract:

	I/O devices.

--*/


#include <carbsup.h>
#include "obp.h"
#include "iop.h"



NTSTATUS
IoCreateDevice(
	__in PDRIVER_OBJECT DriverObject,
	__in PUNICODE_STRING DeviceName,
	__out PDEVICE_OBJECT* DeviceObject
)
{
	NTSTATUS ntStatus;
	OBJECT_ATTRIBUTES ObjectAttributes;

	ObjectAttributes.Attributes = OBJ_PERMANENT;//hm.
	ObjectAttributes.ObjectName = DeviceName;

	ntStatus = ObpCreateObject( DeviceObject, &ObjectAttributes, ObjectTypeDevice );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	( *DeviceObject )->DriverObject = DriverObject;

	return STATUS_SUCCESS;
}

