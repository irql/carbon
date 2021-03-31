


#include <carbsup.h>
#include "iop.h"

VOID
IopCleanupDevice(
    _In_ PDEVICE_OBJECT Device
)
{
    Device;

}

NTSTATUS
IoCreateDevice(
    _In_  PDRIVER_OBJECT  DriverObject,
    _In_  ULONG64         DeviceExtensionSize,
    _In_  PUNICODE_STRING DeviceName,
    _In_  ULONG32         DeviceCharacteristics,
    _Out_ PDEVICE_OBJECT* DeviceObject
)
{
    NTSTATUS ntStatus;
    OBJECT_ATTRIBUTES DeviceAttributes = { { 0 }, { 0 }, OBJ_PERMANENT_OBJECT };

    RtlCopyMemory( &DeviceAttributes.ObjectName, DeviceName, sizeof( UNICODE_STRING ) );
    ntStatus = ObCreateObject( DeviceObject,
                               IoDeviceObject,
                               &DeviceAttributes,
                               sizeof( DEVICE_OBJECT ) + DeviceExtensionSize );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    ( *DeviceObject )->DriverObject = DriverObject;
    ( *DeviceObject )->DeviceExtension = DeviceExtensionSize == 0 ? NULL : ( PVOID )( ( *DeviceObject ) + 1 );
    ( *DeviceObject )->DeviceExtensionSize = DeviceExtensionSize;
    ( *DeviceObject )->DeviceCharacteristics = DeviceCharacteristics | DEV_INITIALIZING;
    ( *DeviceObject )->StackLength = 1;
    ( *DeviceObject )->DeviceLink = NULL;

    return STATUS_SUCCESS;
}

PIRP
IoAllocateIrp(
    _In_ ULONG32 StackCount
)
{
    PIRP Request;

    Request = MmAllocatePoolWithTag( NonPagedPoolZeroed, sizeof( IRP ) + sizeof( IO_STACK_LOCATION ) * StackCount, IO_TAG );

    KeInitializeEvent( &Request->Event, FALSE );

    return Request;
}

VOID
IoFreeIrp(
    _In_ PIRP Request
)
{

    MmFreePoolWithTag( Request, IO_TAG );
}

NTSTATUS
IoCallDriver(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP           Request
)
{
    NTSTATUS ntStatus;
    PDEVICE_OBJECT CurrentDevice;

    KeSignalEvent( &Request->Event, FALSE );

    Request->CurrentStack = 0;
    CurrentDevice = DeviceObject;
    do {

        //IoCopyCurrentIrpStackToNext( Request );

        ntStatus = CurrentDevice->DriverObject->MajorFunction
            [ Request->StackLocation[ Request->CurrentStack ].MajorFunction ](
                CurrentDevice,
                Request );
        if ( !NT_SUCCESS( ntStatus ) ) {

            // uh oh
        }
        if ( KeQueryEvent( &Request->Event ) ) {

            break;
        }


        Request->CurrentStack++;
        CurrentDevice = CurrentDevice->DeviceLink;
    } while ( CurrentDevice != NULL );

    return ntStatus;
}

VOID
IoAttachDevice(
    _In_ PDEVICE_OBJECT High,
    _In_ PDEVICE_OBJECT Low
)
{
    //
    // Expect a bugcheck if you pass a device
    // that is not the lowest level device on 
    // a device stack.
    //
    // This makes it so that when IoCallDriver is
    // called, it will be sending a request from
    // the lowest driver, going upwards. Low
    // will become the new lowest device on the stack
    // and High will be pushed.
    //
    // This also makes it so that the stack length of 
    // the lowest is + 1 of the one above it, so that there
    // are no problems when calling a device inside a device
    // stack.
    //

    Low->DeviceLink = High;
    Low->StackLength = High->StackLength + 1;
}
