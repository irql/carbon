


#include <carbsup.h>

#define PCISYSAPI DLLEXPORT
#include "pci.h"
#include "pcip.h"

#define PCI_TAG ' ICP'
#define DEFAULT_DEVICE_STRING   L"\\Device\\PCI_%.8ull"
//                                \??\ PCI#VendorId (4), 
//                                                    DeviceId (4), 
//                                                              {ClassCode (2), Subclass (2), ProgIf (2)}, 
//                                                                                  {Bus (2), Device (2), Function (2)}
#define DEFAULT_LINK_STRING     L"\\??\\PCI#VEN_%.4UL&DEV_%.4UL&{%.2UL,%.2UL,%.2UL}&{%.2UL,%.2UL,%.2UL}"

ULONG64 PciDeviceCount;
struct {
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING LinkName;
} *PciTable;

VOID
PciReadBase(
    _In_  PPCI_DEVICE Device,
    _Out_ PPCI_BASE   Base,
    _In_  ULONG32     Index
)
{
    ULONG32 Offset = FIELD_OFFSET( PCI_STANDARD_DEVICE, Bar ) + ( ( ULONG )Index * sizeof( ULONG ) );
    PCI_BAR Address = { 0 };
    PCI_BAR Mask = { 0 };

    Address.Lower = PciRead32( Device, Offset );
    PciWrite32( Device, Offset, ( ULONG32 )-1 );
    Mask.Lower = PciRead32( Device, Offset );
    PciWrite32( Device, Offset, Address.Lower );

    if ( Address.LongWidth && !Address.Mmio ) {

        Offset += sizeof( ULONG32 );

        Address.Upper = PciRead32( Device, Offset );
        PciWrite32( Device, Offset, ( ULONG32 )-1 );
        Mask.Upper = PciRead32( Device, Offset );
        PciWrite32( Device, Offset, Address.Upper );
    }

    Base->Base = Address.Long & ~( Address.Mmio ? 0x3ULL : 0xFULL );
    Base->Size = Mask.Long & ~( Address.Mmio ? 0x3ULL : 0xFULL );
    Base->Size = ~Base->Size + 1;
    Base->Flags = Address.Lower & ( Address.Mmio ? 0x3ULL : 0xFULL );
}

VOID
PciSetIoEnable(
    _In_ PPCI_DEVICE Device,
    _In_ BOOLEAN     Enable
)
{
    //
    //  0x7 is mem space enable, io space enable, bus mastering.
    //

    if ( Enable ) {

        Device->PciDevice.Header.Command |= 0x7;
    }
    else {

        Device->PciDevice.Header.Command &= ~0x7;
    }

    PciWrite16( Device, FIELD_OFFSET( PCI_DEVICE_HEADER, Command ), Device->PciDevice.Header.Command );
}

VOID
PciReadConfig(
    _In_  PPCI_DEVICE Device,
    _In_  ULONG32     Offset,
    _Out_ PVOID       Buffer,
    _In_  ULONG32     Length
)
{
    while ( Length-- ) {

        ( ( PUCHAR )Buffer )[ Length ] = PciRead8( Device, Offset + Length );
    }
}

NTSTATUS
PciReadCap(
    _In_  PPCI_DEVICE Device,
    _In_  UCHAR       CapId,
    _Out_ PULONG32    Offset
)
{

    ULONG32 CapOffset;

    if ( ( Device->PciDevice.Header.Status & ( 1 << 4 ) ) == 0 ) {

        return STATUS_DEVICE_FAILED;
    }

    CapOffset = Device->PciDevice.CapabilititesPointer;

    do {

        if ( PciRead8( Device, CapOffset ) == CapId ) {

            *Offset = CapOffset;
            return STATUS_SUCCESS;
        }
        else {

            CapOffset = PciRead8( Device, CapOffset + 1 );
        }

    } while ( CapOffset != 0 );

    return STATUS_NOT_FOUND;
}

VOID
PciQueryDevices(
    _In_ PWSTR              Format,
    _In_ PKPCI_QUERY_DEVICE QueryProcedure,
    _In_ PVOID              QueryContext
)
{
    //
    // Format should be formatted like
    // \??\PCI#VEN_????&DEV_????&{01,02,03}&{??,??,??}
    //

    ULONG64 CurrentDevice;
    ULONG64 CurrentChar;
    BOOLEAN Found;

    for ( CurrentDevice = 0; CurrentDevice < PciDeviceCount; CurrentDevice++ ) {

        Found = TRUE;
        for (
            CurrentChar = 0;
            Format[ CurrentChar ] != 0 &&
            CurrentChar < ( PciTable[ CurrentDevice ].LinkName.Length / sizeof( WCHAR ) );
            CurrentChar++ ) {

            if ( Format[ CurrentChar ] == '?' ) {
                continue;
            }

            if ( Format[ CurrentChar ] != PciTable[ CurrentDevice ].LinkName.Buffer[ CurrentChar ] ) {
                Found = FALSE;
                break;
            }
        }

        if ( Found && !QueryProcedure(
            &PciTable[ CurrentDevice ].LinkName,
            PciTable[ CurrentDevice ].DeviceObject,
            QueryContext ) ) {

            return;
        }
    }
}

NTSTATUS
DriverLoad(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    DriverObject;
    NTSTATUS ntStatus;
    ULONG32 DeviceCount;
    ULONG32 Bus;
    ULONG32 Device;
    ULONG32 Function;
    ULONG32 HeaderByte;
    PCI_DEVICE PciDevice;
    PPCI_DEVICE Extension;
    PDEVICE_OBJECT PciDeviceObject;
    UNICODE_STRING DeviceName;
    UNICODE_STRING SymbolicLink;

    DeviceCount = 0;
    for ( Bus = 0; Bus < PCI_MAX_BUSES; Bus++ ) {
        for ( Device = 0; Device < PCI_MAX_DEVICES; Device++ ) {
            for ( Function = 0; Function < PCI_MAX_FUNCTIONS; Function++ ) {

                PciDevice.Bus = Bus;
                PciDevice.Device = Device;
                PciDevice.Function = Function;
                if ( PciRead16( &PciDevice, FIELD_OFFSET( PCI_DEVICE_HEADER, VendorId ) ) == 0xFFFF ) {

                    continue;
                }
                DeviceCount++;
            }
        }
    }

    PciDeviceCount = DeviceCount;
    PciTable = MmAllocatePoolWithTag( NonPagedPoolZeroed, DeviceCount * sizeof( *PciTable ), PCI_TAG );

    DeviceCount = 0;
    for ( Bus = 0; Bus < PCI_MAX_BUSES; Bus++ ) {
        for ( Device = 0; Device < PCI_MAX_DEVICES; Device++ ) {
            for ( Function = 0; Function < PCI_MAX_FUNCTIONS; Function++ ) {

                PciDevice.Bus = Bus;
                PciDevice.Device = Device;
                PciDevice.Function = Function;
                if ( PciRead16( &PciDevice, FIELD_OFFSET( PCI_DEVICE_HEADER, VendorId ) ) == 0xFFFF ) {

                    continue;
                }

                DeviceName.Buffer = MmAllocatePoolWithTag( NonPagedPoolZeroed, 256, PCI_TAG );
                DeviceName.MaximumLength = 256;

                RtlFormatBuffer( DeviceName.Buffer, DEFAULT_DEVICE_STRING, DeviceCount );
                DeviceName.Length = ( USHORT )( lstrlenW( DeviceName.Buffer ) * sizeof( WCHAR ) );

                ntStatus = IoCreateDevice( DriverObject,
                                           sizeof( PCI_DEVICE ),
                                           &DeviceName,
                                           0,
                                           &PciDeviceObject );

                if ( !NT_SUCCESS( ntStatus ) ) {
                    // bug check.
                    return ntStatus;
                }

                Extension = PciDeviceObject->DeviceExtension;
                Extension->Bus = Bus;
                Extension->Device = Device;
                Extension->Function = Function;
                for ( HeaderByte = 0; HeaderByte < sizeof( PCI_STANDARD_DEVICE ); HeaderByte++ ) {

                    ( ( PUCHAR )&Extension->PciDevice )[ HeaderByte ] = PciRead8( Extension, HeaderByte );
                }

                SymbolicLink.Buffer = MmAllocatePoolWithTag( NonPagedPool, 256, PCI_TAG );
                SymbolicLink.MaximumLength = 256;
                SymbolicLink.Length = 0;

                RtlFormatBuffer(
                    SymbolicLink.Buffer,
                    DEFAULT_LINK_STRING,
                    Extension->PciDevice.Header.VendorId,
                    Extension->PciDevice.Header.DeviceId,
                    Extension->PciDevice.Header.ClassCode,
                    Extension->PciDevice.Header.SubClass,
                    Extension->PciDevice.Header.Prog_IF,
                    Bus, Device, Function );

                SymbolicLink.Length = ( USHORT )( lstrlenW( SymbolicLink.Buffer ) * sizeof( WCHAR ) );

                //RtlDebugPrint( L"mhh, %s\n", SymbolicLink.Buffer );

                IoCreateSymbolicLink( &SymbolicLink, &DeviceName );

                PciTable[ DeviceCount ].DeviceObject = PciDeviceObject;
                PciTable[ DeviceCount ].LinkName.Buffer = SymbolicLink.Buffer;
                PciTable[ DeviceCount ].LinkName.Length = SymbolicLink.Length;
                PciTable[ DeviceCount ].LinkName.MaximumLength = SymbolicLink.MaximumLength;

                DeviceCount++;
            }
        }
    }

    return STATUS_SUCCESS;
}
