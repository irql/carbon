


#include <carbsup.h>
#include "../pci/pci.h"
#include "ide.h"

PIO_INTERRUPT IdeIrqPrimary;
PIO_INTERRUPT IdeIrqSecondary;
KEVENT        IdeIrqEventPrimary = { 0 };
KEVENT        IdeIrqEventSecondary = { 0 };
KMUTEX        IdeIrqLockPrimary = { 0 };
KMUTEX        IdeIrqLockSecondary = { 0 };

BOOLEAN
IdeIrqService(
    _In_ PKINTERRUPT Interrupt,
    _In_ PKEVENT     IrqEvent
)
{
    Interrupt;

    //RtlDebugPrint( L"IrqEvent\n" );
    KeSignalEvent( IrqEvent, TRUE );
    return FALSE;
}

BOOLEAN
IdeInitializeDevice(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT PciDevice,
    _In_ PKIDE_CONTROL  Control,
    _In_ PKIDE_CHANNEL  Channel,
    _In_ BOOLEAN        Master,
    _In_ PKEVENT        IrqEvent,
    _In_ PKMUTEX        IrqLock
)
{
    UCHAR Status;
    ULONG32 CurrentWord;
    BOOLEAN Packet;
    PKIDE_DEVICE Ide;
    PDEVICE_OBJECT DriveDevice;
    UNICODE_STRING DriveName;

    Packet = IDE_DEV_ATA;

    IdeWrite( Channel, ATA_REG_CONTROL, 2 );

    IdeWrite( Channel, ATA_REG_HDDEVSEL, 0x80 | 0x20 | ( Master << 4 ) );
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
    } while ( TRUE );

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
                    0,
                    &DriveDevice );

    IoAttachDevice( PciDevice, DriveDevice );

    Ide = DriveDevice->DeviceExtension;
    Ide->Type = IDE_DEV_ATAPI;
    Ide->Master = Master;
    Ide->Control = Control;
    Ide->Channel = Channel;
    KeInitializeMutex( &Ide->Lock );

    for ( CurrentWord = 0; CurrentWord < 256; CurrentWord++ ) {
        Ide->Identity.Buffer[ CurrentWord ] = __inword( Channel->Base + ATA_REG_DATA );
    }

    for ( CurrentWord = 0; CurrentWord < 20; CurrentWord++ ) {
        Ide->Identity.ModelNumber[ CurrentWord ] = _byteswap_ushort( Ide->Identity.ModelNumber[ CurrentWord ] );
    }
    Ide->Identity.ModelNumber[ 19 ] = 0;

    if ( ( Ide->Identity.CommandSets1 & ( 1 << 26 ) ) == ( 1 << 26 ) ) {
        Ide->Flags |= IDE_LBA48;
    }
    else if ( ( Ide->Identity.Capabilities & ( 1 << 9 ) ) == ( 1 << 9 ) ) {
        Ide->Flags |= IDE_LBA28;
    }
    else {
        Ide->Flags |= IDE_CHS;
    }

    if ( ( ( PPCI_DEVICE )PciDevice->DeviceExtension )->PciDevice.Header.Prog_IF & PIR_BM_SUPPORT &&
         FALSE ) {
        // BM DMA is disabled because
        // it is significantly slower as well
        // as not being worth even writing.

        Ide->Flags |= IDE_DMA;
        PciSetIoEnable( ( PPCI_DEVICE )PciDevice->DeviceExtension, TRUE );
        MmDmaCreateAdapter( DriveDevice,
                            0xFFFF0000,
                            MmDmaPrepared,
                            MmCacheWriteBack,
                            0x10000,
                            &Ide->DmaAdapter );
        Ide->IrqEvent = IrqEvent;
        Ide->IrqLock = IrqLock;
    }

    if ( Ide->Identity.CommandSets1 & ( 1 << 26 ) ) {
        Ide->SectorCount = Ide->Identity.MaxLogicalBlockAddressExt & 0xFFFFFFFFFFFF;
    }
    else {
        Ide->SectorCount = Ide->Identity.MaxLogicalBlockAddress & 0xFFFFFFF;
    }

    Ide->Geometry.SectorSize = 512;
    Ide->Geometry.SectorsPerTrack = Ide->Identity.SectorsPerTrack;
    Ide->Geometry.Heads = Ide->Identity.Heads;
    Ide->Geometry.Cylinders = Ide->Identity.Cylinders;

    Ide->BootSector = MmAllocatePoolWithTag( NonPagedPool, 512, IDE_TAG );
    Ide->BootStatus = IdeAccess( DriveDevice,
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
    PKIDE_DEVICE Ide;

    Ide = DeviceObject->DeviceExtension;

    if ( Length % 0x200 != 0 ) {

        return STATUS_INVALID_BUFFER;
    }

    Length /= 0x200;

    if ( Ide->Flags & IDE_DMA ) {

        while ( Length >= 0x80 ) {
            ntStatus = IdeAccessInternal(
                DeviceObject,
                Write,
                BlockAddress,
                0x80,
                Buffer );

            if ( !NT_SUCCESS( ntStatus ) ) {

                return ntStatus;
            }

            Length -= 0x80;
            BlockAddress += 0x80;

            Buffer = ( PUCHAR )Buffer + 0x80 * 512;
        }
    }
    else {

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

    PKIDE_DEVICE Ide;

    Ide = DeviceObject->DeviceExtension;

    UCHAR Command;
    UCHAR BlockAddressIo[ 6 ];
    UCHAR Head;

    UCHAR Sector;
    USHORT Cylinder;

    BM_PRDT_ENTRY Prdt;
    ULONG64 PrdtVirtual;
    ULONG64 PrdtLogical;
    UCHAR Status;

    if ( BlockAddress >= Ide->SectorCount ) {

        return STATUS_INVALID_ADDRESS;
    }

    NT_ASSERT( ( __readeflags( ) & 0x200 ) == 0x200 );

    //RtlDebugPrint( L"Read_%d_%d\n", BlockAddress, Length );

    if ( Ide->Flags & IDE_LBA48 ) {

        BlockAddressIo[ 0 ] = ( BlockAddress ) & 0xff;
        BlockAddressIo[ 1 ] = ( BlockAddress >> 8 ) & 0xff;
        BlockAddressIo[ 2 ] = ( BlockAddress >> 16 ) & 0xff;
        BlockAddressIo[ 3 ] = ( BlockAddress >> 24 ) & 0xff;
        BlockAddressIo[ 4 ] = ( BlockAddress >> 32 ) & 0xff;
        BlockAddressIo[ 5 ] = ( BlockAddress >> 40 ) & 0xff;
        Head = 0;

        if ( Ide->Flags & IDE_DMA ) {
            Command = Write ? ATA_CMD_WRITE_DMA_EXT : ATA_CMD_READ_DMA_EXT;
        }
        else {
            Command = Write ? ATA_CMD_WRITE_PIO_EXT : ATA_CMD_READ_PIO_EXT;
        }
    }
    else if ( Ide->Flags & IDE_LBA28 ) {

        BlockAddressIo[ 0 ] = ( BlockAddress ) & 0xff;
        BlockAddressIo[ 1 ] = ( BlockAddress >> 8 ) & 0xff;
        BlockAddressIo[ 2 ] = ( BlockAddress >> 16 ) & 0xff;
        Head = ( BlockAddress >> 24 ) & 0xf;

        if ( Ide->Flags & IDE_DMA ) {
            Command = Write ? ATA_CMD_WRITE_DMA : ATA_CMD_READ_DMA;
        }
        else {
            Command = Write ? ATA_CMD_WRITE_PIO : ATA_CMD_READ_PIO;
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
            Command = Write ? ATA_CMD_WRITE_DMA : ATA_CMD_READ_DMA;
        }
        else {
            Command = Write ? ATA_CMD_WRITE_PIO : ATA_CMD_READ_PIO;
        }
    }

    KeAcquireMutex( &Ide->Lock );

    while ( IdeRead( Ide->Channel, ATA_REG_STATUS ) & ATA_SR_BSY )
        ;

    if ( Ide->Flags & IDE_DMA ) {

        MmDmaAllocateBuffer( Ide->DmaAdapter,
                             Length << 9,
                             &PrdtLogical,
                             &PrdtVirtual );
        Prdt.BaseAddress = ( ULONG32 )PrdtLogical;
        Prdt.ByteCount = ( ULONG32 )Length << 9;
        Prdt.EndOfTable = 1;

        KeAcquireMutex( Ide->IrqLock );
        KeSignalEvent( Ide->IrqEvent, FALSE );

        BmWrite( Ide->Channel, BM_REG_COMMAND, 0 );
        BmWrite( Ide->Channel,
                 BM_REG_PRDT_ADDRESS,
                 ( ULONG32 )MmGetLogicalAddress( ( ULONG64 )&Prdt ) );

        IdeWrite( Ide->Channel, ATA_REG_CONTROL, 0 );
    }
    else {

        IdeWrite( Ide->Channel, ATA_REG_CONTROL, 2 );
    }

    IdeWrite( Ide->Channel, ATA_REG_HDDEVSEL, 0x80 | 0x20 | ( Ide->Master << 4 ) | Head | ( Ide->Flags & IDE_CHS ? 0 : 0x40 ) );//( 0x40 * ( ( Ide->Flags & IDE_CHS ) == 0 ) ) );

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

        BmWrite( Ide->Channel, BM_REG_COMMAND, BM_CMD_START | ( Write ? 0 : BM_CMD_WRITE ) );

        do {

            Status = ( UCHAR )BmRead( Ide->Channel, BM_REG_STATUS );

            if ( Status & BM_SR_TRC ||
                 Status & BM_SR_TRC_E ) {

                break;
            }

            if ( Status & BM_SR_ERR ) {

                break;
            }

            KeWaitForSingleObject( Ide->IrqEvent, 10 );

        } while ( TRUE );

        //if ( !KeQueryEvent( Ide->IrqEvent ) )
            //RtlDebugPrint( L"Signal %d\n", KeQueryEvent( Ide->IrqEvent ) );

        KeSignalEvent( Ide->IrqEvent, FALSE );

        BmWrite( Ide->Channel, BM_REG_COMMAND, 0 );

        KeReleaseMutex( Ide->IrqLock );

        if ( Status & BM_SR_ERR ) {

            RtlDebugPrint( L"BmStatus: %ul\n", Status );
            return STATUS_UNSUCCESSFUL;
        }

        Status = IdeRead( Ide->Channel, ATA_REG_STATUS );

        if ( Status & ATA_SR_ERR ||
             Status & ATA_SR_DF ) {

            RtlDebugPrint( L"Not this disk..\n" );
            return STATUS_UNSUCCESSFUL;
        }

        RtlCopyMemory( Buffer, ( PVOID )PrdtVirtual, Length << 9 );

    }
    else {
        if ( Write ) {

            NT_ASSERT( FALSE );
        }
        else {
            while ( Length-- ) {

                while ( IdeRead( Ide->Channel, ATA_REG_STATUS ) & ATA_SR_BSY )
                    ;

                Status = IdeRead( Ide->Channel, ATA_REG_STATUS );

                if ( Status & ATA_SR_ERR ) {

                    KeReleaseMutex( &Ide->Lock );
                    return STATUS_UNSUCCESSFUL;
                }

                if ( Status & ATA_SR_DF ) {

                    KeReleaseMutex( &Ide->Lock );
                    return STATUS_UNSUCCESSFUL;
                }

                if ( ( Status & ATA_SR_DRQ ) == 0 ) {

                    KeReleaseMutex( &Ide->Lock );
                    return STATUS_UNSUCCESSFUL;
                }

                __inwordstring( Ide->Channel->Base + ATA_REG_DATA, Buffer, 256 );

                Buffer = ( PUCHAR )Buffer + 512;
            }
        }
    }

    KeReleaseMutex( &Ide->Lock );
    return STATUS_SUCCESS;
}
