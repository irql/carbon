


#include <carbsup.h>
#include "../dxgi.h"
#include "ddi.h"

ULONG64 DdAdapterCount = 0;

NTSTATUS
DdCreateAdapter(
    _In_  PDEVICE_OBJECT  DeviceObject,
    _Out_ PDEVICE_OBJECT* DdiDeviceObject
)
{
    //
    // Not sure how I associate monitors/outputs, adapters and other shit
    // I guess first come, first serve for now? eventually implement some d3dhal
    // apis to allow drivers to enumerate outputs/monitors? It would also be
    // more convenient to have interfaces for the outputs not just the adapter,
    // since drawing to an adapter is kinda weird.
    //
    // implement some adapter apis into the kernel, for hardware enumeration
    // or other shit like that
    //

    UNICODE_STRING AdapterDeviceName ={ 0 };

    AdapterDeviceName.Buffer = MmAllocatePoolWithTag( NonPagedPoolZeroed, 512, DXGI_TAG );
    AdapterDeviceName.MaximumLength = 512;

    RtlFormatBuffer( AdapterDeviceName.Buffer, L"\\Device\\DdAdapter%d", DdAdapterCount );
    AdapterDeviceName.Length = ( USHORT )lstrlenW( AdapterDeviceName.Buffer ) * sizeof( WCHAR );

    IoCreateDevice( DdiDriverObject,
                    sizeof( D3DHAL ),
                    &AdapterDeviceName,
                    DEV_EXCLUSIVE,
                    DdiDeviceObject );

    IoAttachDevice( DeviceObject, *DdiDeviceObject );

    DdAdapterCount++;

    return DXSTATUS_SUCCESS;
}
