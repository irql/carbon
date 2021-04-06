


#include <carbsup.h>
#include "../pci/pci.h"
#include "ide.h"

EXTERN PIO_INTERRUPT IdeIrqPrimary;
EXTERN PIO_INTERRUPT IdeIrqSecondary;
EXTERN KEVENT        IdeIrqEventPrimary;
EXTERN KEVENT        IdeIrqEventSecondary;
EXTERN KMUTEX        IdeIrqLockPrimary;
EXTERN KMUTEX        IdeIrqLockSecondary;

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

    //10.6. Native PCI Mode Considerations 

    // CHECK IF PCI NATIVE MODE USING PROGIF

    Control = MmAllocatePoolWithTag( NonPagedPoolZeroed, sizeof( KIDE_CONTROL ), IDE_TAG );
    PciIde = DeviceObject->DeviceExtension;

    for ( CurrentBar = 0; CurrentBar < 5; CurrentBar++ ) {

        PciReadBase( PciIde, &Bar[ CurrentBar ], CurrentBar );
    }

    Control->Primary.Base = Bar[ 0 ].Base == 0 ? 0x1F0 : ( USHORT )Bar[ 0 ].Base;
    Control->Primary.Control = Bar[ 1 ].Base == 0 ? 0x3F6 : ( USHORT )Bar[ 1 ].Base;
    Control->Primary.BusMaster = ( USHORT )Bar[ 4 ].Base;

    Control->Secondary.Base = Bar[ 2 ].Base == 0 ? 0x170 : ( USHORT )Bar[ 2 ].Base;
    Control->Secondary.Control = Bar[ 3 ].Base == 0 ? 0x376 : ( USHORT )Bar[ 3 ].Base;
    Control->Secondary.BusMaster = ( USHORT )Bar[ 4 ].Base + 8;

    IdeInitializeDevice( DriverObject,
                         DeviceObject,
                         Control,
                         &Control->Primary,
                         FALSE,
                         &IdeIrqEventPrimary,
                         &IdeIrqLockPrimary );
    IdeInitializeDevice( DriverObject,
                         DeviceObject,
                         Control,
                         &Control->Primary,
                         TRUE,
                         &IdeIrqEventPrimary,
                         &IdeIrqLockPrimary );
    IdeInitializeDevice( DriverObject,
                         DeviceObject,
                         Control,
                         &Control->Secondary,
                         FALSE,
                         &IdeIrqEventSecondary,
                         &IdeIrqLockSecondary );
    IdeInitializeDevice( DriverObject,
                         DeviceObject,
                         Control,
                         &Control->Secondary,
                         TRUE,
                         &IdeIrqEventSecondary,
                         &IdeIrqLockSecondary );

    return TRUE;
}

NTSTATUS
DriverLoad(
    _In_ PDRIVER_OBJECT DriverObject
)
{

    //http://www.bswd.com/pciide.pdf
    //http://bswd.com/idems100.pdf

    //http://bos.asmhackers.net/docs/ata/docs/29860001.pdf

    //https://web.archive.org/web/20150810230806/http://www.t13.org/Documents/UploadedDocuments/project/d1510r1-Host-Adapter.pdf

    KAPIC_REDIRECT Redirect;
    OBJECT_ATTRIBUTES Irq = { 0 };

    // compat mode irq's 
    // native mode should be detected and handled.

    KeInitializeEvent( &IdeIrqEventPrimary, FALSE );
    KeInitializeEvent( &IdeIrqEventSecondary, FALSE );

    KeInitializeMutex( &IdeIrqLockPrimary );
    KeInitializeMutex( &IdeIrqLockSecondary );

    IoConnectInterrupt( &IdeIrqPrimary,
        ( KSERVICE_ROUTINE )IdeIrqService,
                        &IdeIrqEventPrimary,
                        0x50,
                        5,
                        &Irq );
    Redirect.Lower = 0;
    Redirect.Upper = 0;
    Redirect.InterruptVector = 0x50;
    Redirect.DeliveryMode = DeliveryModeEdge;
    Redirect.DestinationMode = DestinationModePhysical;
    Redirect.Destination = 0;
    HalApicRedirectIrq( 14, &Redirect );

    IoConnectInterrupt( &IdeIrqSecondary,
        ( KSERVICE_ROUTINE )IdeIrqService,
                        &IdeIrqEventSecondary,
                        0x51,
                        5,
                        &Irq );
    Redirect.Lower = 0;
    Redirect.Upper = 0;
    Redirect.InterruptVector = 0x51;
    Redirect.DeliveryMode = DeliveryModeEdge;
    Redirect.DestinationMode = DestinationModePhysical;
    Redirect.Destination = 0;
    HalApicRedirectIrq( 15, &Redirect );

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
            Request->IoStatus.Status = IdeAccess( DeviceObject,
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
