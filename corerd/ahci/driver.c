


#include <carbsup.h>
#include "../pci/pci.h"
#include "ahci.h"

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
AhciIrqService(
    _In_ PKINTERRUPT Interrupt,
    _In_ PKEVENT     IrqEvent
)
{
    Interrupt;
    IrqEvent;

    RtlDebugPrint( L"AhciIrqService!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" );

    return TRUE;
}

BOOLEAN
AhciDevice(
    _In_ PUNICODE_STRING LinkName,
    _In_ PDEVICE_OBJECT  DeviceObject,
    _In_ PDRIVER_OBJECT  DriverObject
)
{
    LinkName;
    NTSTATUS ntStatus;
    PPCI_DEVICE PciAhci;
    PCI_BASE Bar5;
    PKAHCI_CONTROL Control;
    ULONG64 Port;
    ULONG64 Type;
    ULONG64 Status;
    OBJECT_ATTRIBUTES IrqLine = { 0 };
    //KMSI_CAP MsiCap;
    ULONG32 MsiCapOffset;

    Control = MmAllocatePoolWithTag( NonPagedPoolZeroed, sizeof( KAHCI_CONTROL ), AHCI_TAG );
    PciAhci = DeviceObject->DeviceExtension;

    ntStatus = PciReadCap( PciAhci,
                           PCI_MSI_CAP_ID,
                           &MsiCapOffset );
    //MsiCap.MessageControl = 0x90;
    //PciWrite16( PciAhci, MsiCapOffset + FIELD_OFFSET( KMSI_CAP, MessageData ), 0x90 );
    //PciWrite16( PciAhci, MsiCapOffset + FIELD_OFFSET( KMSI_CAP, MessageControl ), 0x1 );

    //RtlDebugPrint( L"ahci caps ptr: %d %ul\n", MsiCap.CapId, ntStatus );

    PciReadBase( PciAhci, &Bar5, 5 );

    Control->LogicalAddress = Bar5.Base;
    Control->Io = MmMapIoSpaceSpecifyCache( Control->LogicalAddress,
                                            sizeof( KAHCI_HBA_MMIO ),
                                            MmCacheUncacheable );

    IoConnectInterrupt( &Control->InterruptObject,
        ( KSERVICE_ROUTINE )AhciIrqService,
                        NULL,
                        0x90,
                        DISPATCH_LEVEL,
                        &IrqLine );
#if 0
    // need to do msi's
    Redirect.Lower = 0;
    Redirect.Upper = 0;
    Redirect.InterruptVector = 0x90;
    Redirect.DeliveryMode = DeliveryModeEdge;
    Redirect.DestinationMode = DestinationModePhysical;
    HalApicRedirectIrq( PciAhci->PciDevice.InterruptLine, &Redirect );
#endif

    //RtlDebugPrint( L"IrqLine: %d\n", PciAhci->PciDevice.InterruptLine );

    for ( Port = 0; Port < 32; Port++ ) {

        if ( Control->Io->PortImplemented & ( 1 << Port ) ) {
            Type = AHCI_DEV_NULL;
            Status = Control->Io->Ports[ Port ].SataStatus;

            if ( ( Status & 0x0F ) != HBA_PORT_DET_PRESENT ) {

                continue;
            }

            if ( ( Status & 0xF00 ) != HBA_PORT_IPM_ACTIVE ) {

                continue;
            }

            switch ( Control->Io->Ports[ Port ].Signature ) {
            case SATA_SIG_ATAPI:
                Type = AHCI_DEV_ATAPI;
                break;
            case SATA_SIG_SEMB:
                Type = AHCI_DEV_SEMB;
                break;
            case SATA_SIG_PM:
                Type = AHCI_DEV_PM;
                break;
            default:
                Type = AHCI_DEV_ATA;
                break;
            }

            if ( Type != AHCI_DEV_ATA ) {

                continue;
            }

            //RtlDebugPrint( L"Real port: %ul\n", Port );

            AhciInitializeDevice( DriverObject,
                                  DeviceObject,
                                  Control,
                                  Control->Io->Ports + Port );
        }
    }

    return TRUE;
}

NTSTATUS
DriverLoad(
    _In_ PDRIVER_OBJECT DriverObject
)
{

    //https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/serial-ata-ahci-spec-rev1-3-1.pdf
    //https://www.cl.cam.ac.uk/~djm202/pdf/specifications/pci/PCI_LB3.0_CB-2-6-04.pdf

    PciQueryDevices(
        L"\\??\\PCI#VEN_????&DEV_????&{01,06,01}&{??,??,??}",
        ( PKPCI_QUERY_DEVICE )AhciDevice, DriverObject );

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
    PKAHCI_DEVICE Ahci;

    Current = IoGetCurrentStackLocation( Request );
    Ahci = DeviceObject->DeviceExtension;

    switch ( Current->MajorFunction ) {
    case IRP_MJ_CREATE:

        Request->IoStatus.Status = STATUS_SUCCESS;
        Request->IoStatus.Information = 0;

        break;
    case IRP_MJ_READ:
    case IRP_MJ_WRITE:

        if ( !NT_SUCCESS( Ahci->BootStatus ) ) {

            Request->IoStatus.Status = STATUS_INACCESSIBLE_DEVICE;
            Request->IoStatus.Information = 0;
            IoCompleteRequest( Request );
            return STATUS_SUCCESS;
        }

        if ( Current->Parameters.Read.Offset / 512 > Ahci->SectorCount ) {

            Request->IoStatus.Status = STATUS_INVALID_ADDRESS;
            Request->IoStatus.Information = 0;
        }
        else {
            //RtlDebugPrint( L"[ahci] offset: %d\n", Current->Parameters.Read.Offset / 512 );
            Request->IoStatus.Status = AhciAccess( DeviceObject,
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

        Request->IoStatus.Status = STATUS_INVALID_REQUEST;
        Request->IoStatus.Information = 0;

        break;
    }

    IoCompleteRequest( Request );

    return STATUS_SUCCESS;
}
