


#include <carbsup.h>
#include "dxgi.h"
#include "./ddi/ddi.h"

PDRIVER_OBJECT DdiDriverObject;

NTSTATUS
DriverLoad(
    _In_ PDRIVER_OBJECT DriverObject
);

NTSTATUS
DriverDispatch(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP           Request
);

NTSTATUS
DriverLoad(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    DdiDriverObject = DriverObject;

    //KdPrint( L"** dxgi **\n" );

    __stosq( ( ULONG64* )&DriverObject->MajorFunction, ( ULONG64 )DriverDispatch, IRP_MJ_MAX );

    return STATUS_SUCCESS;
}

NTSTATUS
DriverDispatch(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP           Request
)
{
    PIO_STACK_LOCATION Current;

    Current = IoGetCurrentStackLocation( Request );
    IoCopyCurrentIrpStackToNext( Request );

    //
    // when you can be arsed, add probing and shit like that lol
    //

    //RtlDebugPrint( L"dxgi request.\n" );

    switch ( Current->MajorFunction ) {
    case IRP_MJ_CREATE:
    case IRP_MJ_CLOSE:
    case IRP_MJ_CLEANUP:
        Request->IoStatus.Status = STATUS_SUCCESS;
        Request->IoStatus.Information = 0;
        break;
    case IRP_MJ_CONTROL:
        //RtlDebugPrint( L"dxgi request. %d\n", Current->Parameters.Control.ControlCode );
        switch ( Current->Parameters.Control.ControlCode ) {
        case IOCTL_GET_VERSION:
            // :halfmemeright:
            *( ULONG64* )Request->SystemBuffer1 = DdGetAdapterD3dHal( DeviceObject )->D3dHalVersion;
            break;
        case IOCTL_GET_CAPS:
        case IOCTL_SURFACE_CREATE:
        case IOCTL_SURFACE_DESTROY:
        case IOCTL_CONTEXT_CREATE:
        case IOCTL_CONTEXT_DESTROY:
        case IOCTL_SET_RENDER_TARGET:
        case IOCTL_SET_TEXTURE_STATE:
        case IOCTL_SET_TRANSFORM:
        case IOCTL_SET_MATERIAL:
        case IOCTL_SET_LIGHT_DATA:
        case IOCTL_SET_LIGHT_ENABLED:
        case IOCTL_SHADER_CREATE:
        case IOCTL_SHADER_DESTROY:
        case IOCTL_PRESENT:
        case IOCTL_CLEAR:
        case IOCTL_BLT:
        case IOCTL_STRETCH_BLT:
        case IOCTL_SET_ZRANGE:
        case IOCTL_SET_VIEWPORT:
        case IOCTL_SUBMIT_COMMAND:
            // :halfmemeleft:
            Request->IoStatus.Status = ( ( NTSTATUS( *)( PDEVICE_OBJECT, PVOID ) )(
                ( ( ULONG64* )DdGetAdapterD3dHal( DeviceObject ) )
                [ Current->Parameters.Control.ControlCode ] ) )(
                    DeviceObject->DeviceLink,
                    Request->SystemBuffer1 );
            Request->IoStatus.Information = 0;
            break;
        default:
            Request->IoStatus.Status = STATUS_INVALID_REQUEST;
            Request->IoStatus.Information = 0;
            break;
        }

        break;
    default:

        Request->IoStatus.Status = STATUS_INVALID_REQUEST;
        Request->IoStatus.Information = 0;
        break;
    }

    IoCompleteRequest( Request ); // potentially change this to allow for hardware device querying

    return STATUS_SUCCESS;
}
