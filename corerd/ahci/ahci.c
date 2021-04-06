


#include <carbsup.h>
#include "../pci/pci.h"
#include "ahci.h"

BOOLEAN
AhciInitializeDevice(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PDEVICE_OBJECT  PciDevice,
    _In_ PKAHCI_CONTROL  Control,
    _In_ PKAHCI_HBA_PORT Port
)
{
    PciDevice;

    PKAHCI_DEVICE Ahci;
    PDEVICE_OBJECT DriveDevice;
    UNICODE_STRING DriveName;

    ULONG64 Slot;
    PKAHCI_HBA_COMMAND_HEADER Header;
    PKAHCI_HBA_COMMAND_TABLE Table;
    PFIS_REG_H2D Fis;

    ULONG64 CurrentWord;

    FsRtlCreateDiskDevice( &DriveName );

    IoCreateDevice( DriverObject,
                    sizeof( KAHCI_DEVICE ),
                    &DriveName,
                    0,
                    &DriveDevice );

    //RtlDebugPrint( L"ahci dev create: %s\n", DriveName.Buffer );

    // multiple things attached to one destroys the pci device
    // low field - maybe just remove this field?
    //IoAttachDevice( PciDevice, DriveDevice );

    Ahci = DriveDevice->DeviceExtension;
    Ahci->Control = Control;
    Ahci->Port = Port;
    KeInitializeSemaphore( &Ahci->Semaphore, 32, 0 );

    MmDmaCreateAdapter( DriveDevice,
                        ~0ull,
                        MmDmaSemiPrepared,
                        MmCacheUncacheable,
                        0x400000,
                        &Ahci->DmaAdapter );

    AhciPortInitialize( Ahci );

    Ahci->Port->CommandStatus = ( Ahci->Port->CommandStatus & ~HBA_CMD_ICC ) | HBA_CMD_ICC_IDLE;

    Slot = AhciFreeSlot( Ahci );

    Header = Ahci->CommandList + Slot;
    Header->FisLength = sizeof( FIS_REG_H2D ) / sizeof( ULONG32 );
    Header->Write = FALSE;
    Header->PrdtLength = 1;

    Table = AHCI_CMD_TABLE_ADDR( Ahci->CommandTable, 1, Slot );
    Table->Prdt[ 0 ].BaseAddress = MmGetLogicalAddress( ( ULONG64 )&Ahci->Identity );
    Table->Prdt[ 0 ].ByteCount = 511;
    Table->Prdt[ 0 ].Interrupt = 1;

    Fis = ( PFIS_REG_H2D )&Table->CommandFis;
    RtlZeroMemory( Fis, sizeof( FIS_REG_H2D ) );
    Fis->Type = FIS_TYPE_REG_H2D;
    Fis->CommandRegister = ATA_CMD_IDENTIFY;
    Fis->DeviceRegister = 0;
    Fis->PmPort = 0;
    Fis->Command = 1;

    if ( !AhciEngineSend( Ahci, Slot ) ) {

        RtlDebugPrint( L":nooo:" );
    }

    for ( CurrentWord = 0; CurrentWord < 20; CurrentWord++ ) {
        Ahci->Identity.ModelNumber[ CurrentWord ] = _byteswap_ushort( Ahci->Identity.ModelNumber[ CurrentWord ] );
    }
    Ahci->Identity.ModelNumber[ 19 ] = 0;

    Ahci->SectorCount = Ahci->Identity.MaxLogicalBlockAddressExt & 0xFFFFFFFFFFFF;

    Ahci->Geometry.SectorSize = 512;
    Ahci->Geometry.SectorsPerTrack = Ahci->Identity.SectorsPerTrack;
    Ahci->Geometry.Heads = Ahci->Identity.Heads;
    Ahci->Geometry.Cylinders = Ahci->Identity.Cylinders;

    // only ata devices are even passed to this function
    Ahci->Type = AHCI_DEV_ATA;

    Ahci->BootSector = MmAllocatePoolWithTag( NonPagedPool, 512, AHCI_TAG );
    Ahci->BootStatus = AhciAccess( DriveDevice,
                                   FALSE,
                                   0,
                                   512,
                                   Ahci->BootSector );
    /*
    RtlDebugPrint( L"drive:\n\t%s\n\t%as\n\t%ull\n\t%ul\n",
                   DriveName.Buffer,
                   Ahci->Identity.ModelNumber,
                   Ahci->SectorCount, Ahci->BootStatus );
                   */
    DriveDevice->DeviceCharacteristics &= DEV_INITIALIZING;

    return STATUS_SUCCESS;
}

VOID
AhciEngineStart(
    _In_ PKAHCI_DEVICE Ahci
)
{
    Ahci->Port->CommandStatus &= ~HBA_PxCMD_ST;
    while ( Ahci->Port->CommandStatus & HBA_PxCMD_CR )
        ;
    Ahci->Port->CommandStatus |= HBA_PxCMD_FRE | HBA_PxCMD_ST;
}

VOID
AhciEngineStop(
    _In_ PKAHCI_DEVICE Ahci
)
{
    Ahci->Port->CommandStatus &= ~HBA_PxCMD_ST;
    while ( Ahci->Port->CommandStatus & HBA_PxCMD_CR )
        ;
    Ahci->Port->CommandStatus &= ~HBA_PxCMD_FRE;
}

VOID
AhciPortInitialize(
    _In_ PKAHCI_DEVICE Ahci
)
{
    ULONG64 Command;

    AhciEngineStop( Ahci );

    MmDmaAllocateBuffer( Ahci->DmaAdapter,
                         sizeof( KAHCI_HBA_COMMAND_HEADER ) * 32,
                         &Ahci->Port->CommandListLogical,
                         ( ULONG64* )&Ahci->CommandList );
    RtlZeroMemory( Ahci->CommandList, sizeof( KAHCI_HBA_COMMAND_HEADER ) * 32 );

    MmDmaAllocateBuffer( Ahci->DmaAdapter,
                         sizeof( KAHCI_HBA_FIS ),
                         &Ahci->Port->FisLogical,
                         ( ULONG64* )&Ahci->Fis );
    RtlZeroMemory( Ahci->Fis, sizeof( KAHCI_HBA_FIS ) );

    MmDmaAllocateBuffer( Ahci->DmaAdapter,
                         AHCI_CMD_TABLE_SIZE( 1 ) * 32,
                         &Ahci->CommandTableLogical,
                         ( ULONG64* )&Ahci->CommandTable );
    RtlZeroMemory( Ahci->CommandTable, AHCI_CMD_TABLE_SIZE( 1 ) * 32 );

    for ( Command = 0; Command < 32; Command++ ) {

        Ahci->CommandList[ Command ].PrdtLength = 1;
        Ahci->CommandList[ Command ].CommandTableLogical = ( ULONG64 )AHCI_CMD_TABLE_ADDR( Ahci->CommandTableLogical, 1, Command );//Ahci->CommandTableLogical + Command * sizeof( KAHCI_HBA_COMMAND_TABLE );
    }

    AhciEngineStart( Ahci );
}

ULONG64
AhciFreeSlot(
    _In_ PKAHCI_DEVICE Ahci
)
{
    ULONG32 CurrentSlot;
    ULONG32 SlotMask = Ahci->Port->CommandIssue | Ahci->Port->SataActive;

    for ( CurrentSlot = 0; CurrentSlot < 32; CurrentSlot++ ) {

        if ( ( SlotMask & ( 1 << CurrentSlot ) ) == 0 ) {

            return CurrentSlot;
        }
    }

    return ~0ull;
}

BOOLEAN
AhciEngineSend(
    _In_ PKAHCI_DEVICE Ahci,
    _In_ ULONG64       Slot
)
{
    while ( Ahci->Port->TaskFileData & ( ATA_SR_BSY | ATA_SR_DRQ ) )
        ;

    Ahci->Port->CommandIssue = 1 << Slot;

    do {

        if ( Ahci->Port->InterruptStatus & HBA_PxIS_TFES ) {

            return FALSE;
        }

    } while ( Ahci->Port->CommandIssue & ( 1 << Slot ) );

    return TRUE;
}

NTSTATUS
AhciAccess(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ BOOLEAN        Write,
    _In_ ULONG64        BlockAddress,
    _In_ ULONG64        Length,
    _In_ PVOID          Buffer
)
{
    NTSTATUS ntStatus;
    PKAHCI_DEVICE Ahci;

    Ahci = DeviceObject->DeviceExtension;

    if ( Length % 0x200 != 0 ) {

        return STATUS_INVALID_BUFFER;
    }

    Length /= 0x200;

    while ( Length >= 0x2000 ) {
        ntStatus = AhciAccessInternal( DeviceObject,
                                       Write,
                                       BlockAddress,
                                       0x2000,
                                       Buffer );
        if ( !NT_SUCCESS( ntStatus ) ) {

            return ntStatus;
        }

        Length -= 0x2000;
        BlockAddress += 0x2000;

        Buffer = ( PUCHAR )Buffer + 0x2000 * 512;
    }

    if ( Length > 0 ) {
        ntStatus = AhciAccessInternal( DeviceObject,
                                       Write,
                                       BlockAddress,
                                       Length,
                                       Buffer );
        if ( !NT_SUCCESS( ntStatus ) ) {

            return ntStatus;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
AhciAccessInternal(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ BOOLEAN        Write,
    _In_ ULONG64        BlockAddress,
    _In_ ULONG64        Length,
    _In_ PVOID          Buffer
)
{
    PKAHCI_DEVICE Ahci;

    ULONG64 Slot;
    PKAHCI_HBA_COMMAND_HEADER Header;
    PKAHCI_HBA_COMMAND_TABLE Table;
    PFIS_REG_H2D Fis;

    ULONG64 PrdLogical;
    ULONG64 PrdVirtual;

    /*
    5.6 Transfer Examples

    At any point, the Serial ATA link may be in a Partial or Slumber interface power management state. The
    HBA shall ensure that the link is active before transmitting a FIS on the Serial ATA link (refer to section
    8.3.1.3).

    */

    //RtlDebugPrint( L"ahci access: %d %d\n", BlockAddress, Length );

    Ahci = DeviceObject->DeviceExtension;

    MmDmaAllocateBuffer( Ahci->DmaAdapter,
                         0,
                         &PrdLogical,
                         &PrdVirtual );

    KeAcquireSemaphore( &Ahci->Semaphore );

    Ahci->Port->InterruptStatus = ~0ul;
    Slot = AhciFreeSlot( Ahci );

    NT_ASSERT( Slot != ~0ull );
    NT_ASSERT( Write == FALSE );

    //
    // This driver allows the HBA to perform it's own power management
    // but still sets the state to an idle/active state.
    //

    Ahci->Port->CommandStatus = ( Ahci->Port->CommandStatus & ~HBA_CMD_ICC ) | HBA_CMD_ICC_IDLE;

    Header = Ahci->CommandList + Slot;
    Header->FisLength = sizeof( FIS_REG_H2D ) / sizeof( ULONG32 );
    Header->Write = Write;
    Header->PrdtLength = 1;
    Header->Prefetchable = 1;

    Table = AHCI_CMD_TABLE_ADDR( Ahci->CommandTable, 1, Slot );
    Table->Prdt[ 0 ].BaseAddress = PrdLogical;
    Table->Prdt[ 0 ].ByteCount = ( ULONG32 )( Length << 9 ) - 1;
    Table->Prdt[ 0 ].Interrupt = 1;

    Fis = ( PFIS_REG_H2D )&Table->CommandFis;
    RtlZeroMemory( Fis, sizeof( FIS_REG_H2D ) );
    Fis->Type = FIS_TYPE_REG_H2D;
    Fis->CommandRegister = Write ? ATA_CMD_WRITE_DMA_EXT : ATA_CMD_READ_DMA_EXT;
    Fis->DeviceRegister = 0;
    Fis->PmPort = 0;
    Fis->Command = 1;

    Fis->BlockAddressLower[ 0 ] =  ( BlockAddress ) & 0xff;
    Fis->BlockAddressLower[ 1 ] = ( BlockAddress >> 8 ) & 0xff;
    Fis->BlockAddressLower[ 2 ] = ( BlockAddress >> 16 ) & 0xff;
    Fis->DeviceRegister = 1 << 6; // LBA Mode

    Fis->BlockAddressUpper[ 0 ] = ( BlockAddress >> 24 ) & 0xff;
    Fis->BlockAddressUpper[ 1 ] = ( BlockAddress >> 32 ) & 0xff;
    Fis->BlockAddressUpper[ 2 ] = ( BlockAddress >> 40 ) & 0xff;
    Fis->Count = ( USHORT )Length;

    if ( !AhciEngineSend( Ahci, Slot ) ) {

        RtlDebugPrint( L"ahci dev failure.\n" );
        return STATUS_DEVICE_FAILED;
    }

    KeReleaseSemaphore( &Ahci->Semaphore );

    RtlCopyMemory( Buffer, ( PVOID )PrdVirtual, Length << 9 );

    return STATUS_SUCCESS;
}
