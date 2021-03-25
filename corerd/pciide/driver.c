


#include <carbsup.h>
#include "../pci/pci.h"
#include "ide.h"

NTSTATUS
DriverLoad(
    _In_ PDRIVER_OBJECT DriverObject
);

NTSTATUS
DriverDispatch(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP           Request
);

BOOLEAN
IdeDevice(
    _In_ PUNICODE_STRING LinkName,
    _In_ PDEVICE_OBJECT  DeviceObject,
    _In_ PDRIVER_OBJECT  DriverObject
)
{
    LinkName;
    PPCI_DEVICE PciIde;

    PKIDE_CONTROL Control;
    ULONG32 CurrentBar;
    PCI_BASE Bar[ 5 ];

    //RtlDebugPrint( L"ide: %s\n", LinkName->Buffer );

    Control = MmAllocatePoolWithTag( NonPagedPoolZeroed, sizeof( KIDE_CONTROL ), IDE_TAG );
    PciIde = DeviceObject->DeviceExtension;

    for ( CurrentBar = 0; CurrentBar < 5; CurrentBar++ ) {

        PciReadBase( PciIde, &Bar[ CurrentBar ], CurrentBar );
    }

    Control->Primary.Base = ( USHORT )Bar[ 0 ].Base + 0x1f0 * ( !Bar[ 0 ].Base );
    Control->Primary.Control = ( USHORT )Bar[ 1 ].Base + 0x3f6 * ( !Bar[ 1 ].Base );
    Control->Primary.BusMaster = ( USHORT )Bar[ 3 ].Base;
    Control->Primary.NoInterrupt = ( UCHAR )Bar[ 4 ].Base;

    Control->Secondary.Base = ( USHORT )Bar[ 2 ].Base + 0x170 * ( !Bar[ 2 ].Base );
    Control->Secondary.Control = ( USHORT )Bar[ 3 ].Base + 0x376 * ( !Bar[ 3 ].Base );
    Control->Secondary.BusMaster = ( USHORT )Bar[ 3 ].Base + 8;
    Control->Secondary.NoInterrupt = ( UCHAR )Bar[ 4 ].Base + 8;
    /*
    RtlDebugPrint( L"primary: %ul, %ul %ul %ul\n",
                   Control->Primary.Base,
                   Control->Primary.Control,
                   Control->Primary.BusMaster,
                   Control->Primary.NoInterrupt );
    RtlDebugPrint( L"secondary: %ul, %ul %ul %ul\n",
                   Control->Secondary.Base,
                   Control->Secondary.Control,
                   Control->Secondary.BusMaster,
                   Control->Secondary.NoInterrupt );*/

    IdeWrite( &Control->Primary, ATA_REG_CONTROL, 1 << 1 );
    IdeWrite( &Control->Secondary, ATA_REG_CONTROL, 1 << 1 );

    IdeInitializeDevice( DriverObject, DeviceObject, Control, &Control->Primary, 0 );
    IdeInitializeDevice( DriverObject, DeviceObject, Control, &Control->Primary, 1 );
    IdeInitializeDevice( DriverObject, DeviceObject, Control, &Control->Secondary, 0 );
    IdeInitializeDevice( DriverObject, DeviceObject, Control, &Control->Secondary, 1 );

    return TRUE;
}

NTSTATUS
DriverLoad(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    PciQueryDevices(
        L"\\??\\PCI#VEN_????&DEV_????&{01,01,??}&{??,??,??}",
        ( PKPCI_QUERY_DEVICE )IdeDevice, DriverObject );

    __stosq( ( ULONG64* )&DriverObject->MajorFunction, ( ULONG64 )DriverDispatch, IRP_MJ_MAX );

    return STATUS_SUCCESS;
}

NTSTATUS
DriverDispatch(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP           Request
)
{
    DeviceObject;
    //RtlDebugPrint( L"[pciide] dispatch.\n" );

    PIO_STACK_LOCATION Current;
    PKIDE_DEVICE Ide;

    Current = IoGetCurrentStackLocation( Request );
    Ide = DeviceObject->DeviceExtension;

    switch ( Current->MajorFunction ) {
    case IRP_MJ_CREATE:

        //
        // Something is opening a handle/pointer to
        // the device, we can just mark this successful
        // and complete.
        //

        Request->IoStatus.Status = STATUS_SUCCESS;
        Request->IoStatus.Information = 0;

        break;
    case IRP_MJ_READ:
    case IRP_MJ_WRITE:

        if ( !NT_SUCCESS( Ide->BootStatus ) ) {

            Request->IoStatus.Status = STATUS_INACCESSIBLE_DEVICE;
            Request->IoStatus.Information = 0;
            IoCompleteRequest( Request );
            return STATUS_SUCCESS;
        }

        if ( Current->Parameters.Read.Offset / 512 > Ide->SectorCount ) {

            Request->IoStatus.Status = STATUS_INVALID_ADDRESS;
            Request->IoStatus.Information = 0;
        }
        else {
            //RtlDebugPrint( L"[pciide] offset: %d\n", Current->Parameters.Read.Offset / 512 );
            Request->IoStatus.Status = IdeAccess(
                DeviceObject,
                Current->MajorFunction == IRP_MJ_WRITE,
                Current->Parameters.Read.Offset / 512,
                Current->Parameters.Read.Length,
                Request->SystemBuffer1 );
            Request->IoStatus.Information = NT_SUCCESS( Request->IoStatus.Status ) ? Current->Parameters.Read.Length : 0;
        }

        break;
    case IRP_MJ_CLOSE:
    case IRP_MJ_CLEANUP:

        Request->IoStatus.Status = STATUS_SUCCESS;
        Request->IoStatus.Information = 0;

        break;
    default:

        //
        // The pci.sys driver is unable to handle any irp's,
        // the request is invalid and we should complete it
        // and leave.
        //

        Request->IoStatus.Status = STATUS_INVALID_REQUEST;
        Request->IoStatus.Information = 0;

        break;
    }

    IoCompleteRequest( Request );

    return STATUS_SUCCESS;
}
