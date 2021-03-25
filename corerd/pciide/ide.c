


#include <carbsup.h>
#include "ide.h"

BOOLEAN
IdeInitializeDevice(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT PciDevice,
    _In_ PKIDE_CONTROL  Control,
    _In_ PKIDE_CHANNEL  Channel,
    _In_ UCHAR          Drive
)
{
    STATIC ULONG64 Harddisk = 0;

    UCHAR Status;
    ULONG32 CurrentWord;
    BOOLEAN Packet;
    PKIDE_DEVICE Ide;
    PDEVICE_OBJECT DriveDevice;
    UNICODE_STRING DriveName;

    Packet = FALSE;

    IdeWrite( Channel, ATA_REG_HDDEVSEL, 0x80 | 0x20 | ( Drive << 4 ) );
    IdeWrite( Channel, ATA_REG_COMMAND, ATA_CMD_IDENTIFY );

    if ( IdeRead( Channel, ATA_REG_STATUS ) == 0 ) {
        return FALSE;
    }

    // TODO: Deadlock.
    do {
        Status = IdeRead( Channel, ATA_REG_STATUS );

        if ( Status & ATA_SR_ERR ) {
            Packet = TRUE;
            break;
        }

        if ( ( Status & ATA_SR_BSY ) == 0 &&
            ( Status & ATA_SR_DRQ ) == ATA_SR_DRQ ) {
            break;
        }
    } while ( 1 );

    if ( Packet ) {
        IdeWrite( Channel, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET );

        if ( IdeRead( Channel, ATA_REG_STATUS ) & ATA_SR_ERR ) {

            return FALSE;
        }
    }

    FsRtlCreateDiskDevice( &DriveName );

    IoCreateDevice( DriverObject,
                    sizeof( KIDE_DEVICE ),
                    &DriveName,
                    DEV_BUFFERED_IO,
                    &DriveDevice );

    IoAttachDevice( PciDevice, DriveDevice );

    Ide = DriveDevice->DeviceExtension;
    Ide->Packet = Packet;
    Ide->Drive = Drive;
    Ide->Control = Control;
    Ide->Channel = Channel;

    for ( CurrentWord = 0; CurrentWord < 256; CurrentWord++ ) {
        Ide->Identity.Buffer[ CurrentWord ] = __inword( Channel->Base + ATA_REG_DATA );
    }

    for ( CurrentWord = 0; CurrentWord < 20; CurrentWord++ ) {
        Ide->Identity.ModelNumber[ CurrentWord ] = _byteswap_ushort( Ide->Identity.ModelNumber[ CurrentWord ] );
    }
    Ide->Identity.ModelNumber[ 19 ] &= 0xFF;

    if ( ( Ide->Identity.CommandSets1 & ( 1 << 26 ) ) == ( 1 << 26 ) ) {
        Ide->Flags |= IDE_LBA48;
    }
    else if ( ( Ide->Identity.Capabilities & ( 1 << 9 ) ) == ( 1 << 9 ) ) {
        Ide->Flags |= IDE_LBA28;
    }
    else {
        Ide->Flags |= IDE_CHS;
    }

    if ( Ide->Identity.CommandSets1 & ( 1 << 26 ) ) {
        Ide->SectorCount = Ide->Identity.MaxLogicalBlockAddressExt & 0xFFFFFFFFFFFF;
    }
    else {
        Ide->SectorCount = Ide->Identity.MaxLogicalBlockAddress & 0xFFFFFFF;
    }

    Ide->BootSector = MmAllocatePoolWithTag( NonPagedPool, 512, IDE_TAG );
    Ide->BootStatus = IdeAccess(
        DriveDevice,
        FALSE,
        0,
        512,
        Ide->BootSector );
    /*
    RtlDebugPrint( L"drive:\n\t%s\n\t%as\n\t%ull\n\t%ul\n",
                   DriveName.Buffer,
                   Ide->Identity.ModelNumber,
                   Ide->SectorCount, Ide->BootStatus );*/

    DriveDevice->DeviceCharacteristics &= ~DEV_INITIALIZING;

    Harddisk++;

    return TRUE;
}

NTSTATUS
IdeAccess(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ BOOLEAN        Write,
    _In_ ULONG64        BlockAddress,
    _In_ ULONG64        Length,
    _In_ PVOID          Buffer
)
{
    NTSTATUS ntStatus;

    if ( Length % 0x200 != 0 ) {

        return STATUS_INVALID_ADDRESS;
    }

    Length /= 0x200;

    while ( Length >= 0xFFFF ) {
        ntStatus = IdeAccessInternal(
            DeviceObject,
            Write,
            BlockAddress,
            0xFFFF,
            Buffer );

        if ( !NT_SUCCESS( ntStatus ) ) {

            return ntStatus;
        }

        Length -= 0xFFFF;
        BlockAddress += 0xFFFF;

        Buffer = ( PUCHAR )Buffer + 0xFFFF * 512;
    }

    if ( Length > 0 ) {
        ntStatus = IdeAccessInternal(
            DeviceObject,
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
IdeAccessInternal(
    _In_ PDEVICE_OBJECT   DeviceObject,
    _In_ BOOLEAN          Write,
    _In_ ULONG64          BlockAddress,
    _In_ ULONG64          Length,
    _In_ PVOID            Buffer
)
{
    DeviceObject;
    Write;
    BlockAddress;
    Length;
    Buffer;

    KIRQL PreviousIrql;
    PKIDE_DEVICE Ide;

    Ide = DeviceObject->DeviceExtension;

    //Ide->Packet

    UCHAR Command;
    UCHAR BlockAddressIo[ 6 ];
    UCHAR Head;

    UCHAR Sector;
    USHORT Cylinder;

    if ( BlockAddress > Ide->SectorCount ) {

        return STATUS_INVALID_ADDRESS;
    }

    Ide->Channel->NoInterrupt = 1 << 1;//?????

    IdeWrite( Ide->Channel, ATA_REG_CONTROL, 1 << 1 );

    if ( Ide->Flags & IDE_LBA48 ) {

        BlockAddressIo[ 0 ] = ( BlockAddress ) & 0xff;
        BlockAddressIo[ 1 ] = ( BlockAddress >> 8 ) & 0xff;
        BlockAddressIo[ 2 ] = ( BlockAddress >> 16 ) & 0xff;
        BlockAddressIo[ 3 ] = ( BlockAddress >> 24 ) & 0xff;
        BlockAddressIo[ 4 ] = ( BlockAddress >> 32 ) & 0xff;
        BlockAddressIo[ 5 ] = ( BlockAddress >> 40 ) & 0xff;
        Head = 0;

        if ( Ide->Flags & IDE_DMA ) {
            Command = ATA_CMD_READ_DMA_EXT + Write * 0x10;
        }
        else {
            Command = ATA_CMD_READ_PIO_EXT + Write * 0x10;
        }
    }
    else if ( Ide->Flags & IDE_LBA28 ) {

        BlockAddressIo[ 0 ] = ( BlockAddress ) & 0xff;
        BlockAddressIo[ 1 ] = ( BlockAddress >> 8 ) & 0xff;
        BlockAddressIo[ 2 ] = ( BlockAddress >> 16 ) & 0xff;
        Head = ( BlockAddress >> 24 ) & 0xf;

        if ( Ide->Flags & IDE_DMA ) {
            Command = ATA_CMD_READ_DMA + Write * 2;
        }
        else {
            Command = ATA_CMD_READ_PIO + Write * 0x10;
        }
    }
    else {
        BlockAddress++;

        Sector = ( UCHAR )( BlockAddress % Ide->Geometry.SectorsPerTrack );
        Cylinder = ( USHORT )( ( BlockAddress - Sector ) / ( Ide->Geometry.Heads * Ide->Geometry.SectorsPerTrack ) );
        BlockAddressIo[ 0 ] = Sector;
        BlockAddressIo[ 1 ] = ( UCHAR )Cylinder;
        BlockAddressIo[ 2 ] = ( Cylinder >> 8 ) & 0xff;
        Head = ( UCHAR )( ( BlockAddress - Sector ) % ( Ide->Geometry.Heads * Ide->Geometry.SectorsPerTrack ) / Ide->Geometry.SectorsPerTrack );

        if ( Ide->Flags & IDE_DMA ) {
            Command = ATA_CMD_READ_DMA + Write * 2;
        }
        else {
            Command = ATA_CMD_READ_PIO + Write * 0x10;
        }
    }

    KeAcquireSpinLock( &Ide->IdeLock, &PreviousIrql );

    while ( IdeRead( Ide->Channel, ATA_REG_STATUS ) & ATA_SR_BSY )
        ;


    IdeWrite( Ide->Channel, ATA_REG_HDDEVSEL, 0x80 | 0x20 | ( Ide->Drive << 4 ) | Head | ( 0x40 * ( ( Ide->Flags & IDE_CHS ) == 0 ) ) );

    if ( Ide->Flags & IDE_LBA48 ) {

        IdeWrite( Ide->Channel, ATA_REG_SECCOUNT0, ( Length >> 8 ) & 0xff );
        IdeWrite( Ide->Channel, ATA_REG_LBA0, BlockAddressIo[ 3 ] );
        IdeWrite( Ide->Channel, ATA_REG_LBA1, BlockAddressIo[ 4 ] );
        IdeWrite( Ide->Channel, ATA_REG_LBA2, BlockAddressIo[ 5 ] );
    }

    IdeWrite( Ide->Channel, ATA_REG_SECCOUNT0, Length & 0xff );
    IdeWrite( Ide->Channel, ATA_REG_LBA0, BlockAddressIo[ 0 ] );
    IdeWrite( Ide->Channel, ATA_REG_LBA1, BlockAddressIo[ 1 ] );
    IdeWrite( Ide->Channel, ATA_REG_LBA2, BlockAddressIo[ 2 ] );

    IdeWrite( Ide->Channel, ATA_REG_COMMAND, Command );

    if ( Ide->Flags & IDE_DMA ) {


    }
    else {
        if ( Write ) {

        }
        else {
            while ( Length-- ) {

                while ( IdeRead( Ide->Channel, ATA_REG_STATUS ) & ATA_SR_BSY )
                    ;

                UCHAR Status = IdeRead( Ide->Channel, ATA_REG_STATUS );

                if ( Status & ATA_SR_ERR ) {

                    KeReleaseSpinLock( &Ide->IdeLock, PreviousIrql );
                    return STATUS_UNSUCCESSFUL;
                }

                if ( Status & ATA_SR_DF ) {

                    KeReleaseSpinLock( &Ide->IdeLock, PreviousIrql );
                    return STATUS_UNSUCCESSFUL;
                }

                if ( ( Status & ATA_SR_DRQ ) == 0 ) {

                    KeReleaseSpinLock( &Ide->IdeLock, PreviousIrql );
                    return STATUS_UNSUCCESSFUL;
                }

                __inwordstring( Ide->Channel->Base + ATA_REG_DATA, Buffer, 256 );

                Buffer = ( PUCHAR )Buffer + 512;
            }
        }
    }

    KeReleaseSpinLock( &Ide->IdeLock, PreviousIrql );
    return STATUS_SUCCESS;
}
