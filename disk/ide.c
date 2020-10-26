


#include "driver.h"
#include "controller.h"
#include "ide.h"


VOID
IdeWriteRegister(
	__in PIDE_CHANNEL_REGISTERS Channel,
	__in UCHAR Register,
	__in UCHAR Value
);

UCHAR
IdeReadRegister(
	__in PIDE_CHANNEL_REGISTERS Channel,
	__in UCHAR Register
);

UCHAR
IdeAtaAccess(
	__in PIDE_DISK_DATA Disk,
	__in UCHAR Direction,
	__in ULONG64 LogicalBlockAddress,
	__in USHORT Sectors,
	__in UCHAR* Buffer
);

NTSTATUS
FsIdeAtaRead(
	__in PDISK_OBJECT Disk,
	__in ULONG64 LogicalBlockAddress,
	__in PVOID Buffer,
	__in ULONG Length
);

NTSTATUS
FsIdeAtaWrite(
	__in PDISK_OBJECT Disk,
	__in ULONG64 LogicalBlockAddress,
	__in PVOID Buffer,
	__in ULONG Length
);

VOID
FsIdeDetectDrives(

)
{

	for ( ULONG32 i = 0; i < HalPciDeviceList.DeviceCount; i++ ) {

		/*
			0x1 is IDE Controller subclass.
		*/

		if ( HalPciDeviceList.PciDevices[ i ].PciHeader.ClassCode == PCI_CLASS_CODE_MASS_STORAGE_CONTROLLER &&
			HalPciDeviceList.PciDevices[ i ].PciHeader.SubClass == 0x1 ) {

			PIDE_CONTROLLER Ide = ExAllocatePoolWithTag( sizeof( IDE_CONTROLLER ), TAGEX_IDE );

			Ide->PciDevice = &HalPciDeviceList.PciDevices[ i ];

			PCI_BASE_ADDRESS_REGISTER Bar[ 5 ];

			for ( UCHAR j = 0; j < 5; j++ ) {

				HalPciReadBar( Ide->PciDevice, &Bar[ j ], j );
			}

			Ide->Channels[ ATA_PRIMARY ].Base = ( USHORT )Bar[ 0 ].Base + 0x1f0 * ( !Bar[ 0 ].Base );
			Ide->Channels[ ATA_PRIMARY ].Control = ( USHORT )Bar[ 1 ].Base + 0x3f6 * ( !Bar[ 1 ].Base );
			Ide->Channels[ ATA_PRIMARY ].nIEN = ( UCHAR )Bar[ 4 ].Base + 0;

			Ide->Channels[ ATA_SECONDARY ].Base = ( USHORT )Bar[ 2 ].Base + 0x170 * ( !Bar[ 2 ].Base );
			Ide->Channels[ ATA_SECONDARY ].Control = ( USHORT )Bar[ 3 ].Base + 0x376 * ( !Bar[ 3 ].Base );
			Ide->Channels[ ATA_SECONDARY ].nIEN = ( UCHAR )Bar[ 4 ].Base + 8;


			IdeWriteRegister( &Ide->Channels[ ATA_PRIMARY ], ATA_REG_CONTROL, ( 1 << 1 ) );
			IdeWriteRegister( &Ide->Channels[ ATA_SECONDARY ], ATA_REG_CONTROL, ( 1 << 1 ) );

			for ( UCHAR j = 0, Count = 0; j < 2; j++ ) {
				for ( UCHAR k = 0; k < 2; k++ ) {

					//UCHAR Error = 0, Type = IDE_ATA, Status = 0;
					UCHAR Type = IDE_ATA;
					Ide->Devices[ Count ].Exists = 0;

					IdeWriteRegister( &Ide->Channels[ j ], ATA_REG_HDDEVSEL, 0xA0 | ( k << 4 ) );

					IdeWriteRegister( &Ide->Channels[ j ], ATA_REG_COMMAND, ATA_CMD_IDENTIFY );

					if ( IdeReadRegister( &Ide->Channels[ j ], ATA_REG_STATUS ) == 0 ) {

						continue;
					}

#if 0
					while ( IdeReadRegister( &Ide->Channels[ j ], ATA_REG_STATUS ) & ATA_SR_BSY )
						;
#endif

					while ( 1 ) {
						UCHAR Status = IdeReadRegister( &Ide->Channels[ j ], ATA_REG_STATUS );

						if ( Status & ATA_SR_ERR ) {

							Type = IDE_ATAPI;
							break;
						}

						if ( !( Status & ATA_SR_BSY ) && ( Status & ATA_SR_DRQ ) ) {

							break;
						}

					}

					if ( Type == IDE_ATAPI ) {

						IdeWriteRegister( &Ide->Channels[ j ], ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET );
					}


					for ( ULONG32 l = 0; l < 256; l++ ) {

						( ( USHORT* )&Ide->Devices[ Count ].Identity )[ l ] = __inword( Ide->Channels[ j ].Base + ATA_REG_DATA );//IdeReadRegister( &Ide->Channels[ j ], ATA_REG_DATA );
					}

					Ide->Devices[ Count ].Exists = 1;
					Ide->Devices[ Count ].Type = Type;
					Ide->Devices[ Count ].Channel = j;
					Ide->Devices[ Count ].Drive = k;

					/*
						a) Words (61:60) shall contain the value one greater than the total number of user-addressable logical
							sectors in 28-bit addressing and shall not exceed 0FFFFFFFh. The content of words (61:60) shall
							be greater than or equal to one and less than or equal to 268,435,455.
						b) Words (103:100) shall contain the value one greater than the total number of user-addressable
							logical sectors in 48-bit addressing and shall not exceed 0000FFFFFFFFFFFFh.
						c) The contents of words (61:60) and (103:100) may be affected by the host issuing a SET MAX
							ADDRESS or SET MAX ADDRESS EXT command.
						d) The contents of words (61:60) and (103:100) shall not be used to determine if 48-bit addressing is
							supported. IDENTIFY DEVICE bit 10 word 83 indicates support for 48-bit addressing.

						bit of a hack, checking if the command set supports EXT commands.
					*/

					if ( Ide->Devices[ Count ].Identity.CommandSets1 & ( 1 << 26 ) )
						Ide->Devices[ Count ].Size = Ide->Devices[ Count ].Identity.MaxLogicalBlockAddressExt & 0x0000FFFFFFFFFFFF;
					else
						Ide->Devices[ Count ].Size = Ide->Devices[ Count ].Identity.MaxLogicalBlockAddress & 0x0FFFFFFF;

					for ( ULONG l = 0; l < 40; l += 2 ) {
						UCHAR Temp = Ide->Devices[ Count ].Identity.ModelNumber[ l ];
						Ide->Devices[ Count ].Identity.ModelNumber[ l ] = Ide->Devices[ Count ].Identity.ModelNumber[ l + 1 ];
						Ide->Devices[ Count ].Identity.ModelNumber[ l + 1 ] = Temp;
					}
					Ide->Devices[ Count ].Identity.ModelNumber[ 39 ] = 0;

					DbgPrint( L"drive: model: %a\nsectors: %d, cylinders: %d, heads: %d, spt: %d, iobase: %x\n",
						Ide->Devices[ Count ].Identity.ModelNumber,
						Ide->Devices[ Count ].Size,
						Ide->Devices[ Count ].Identity.Cylinders,
						Ide->Devices[ Count ].Identity.Heads,
						Ide->Devices[ Count ].Identity.SectorsPerTrack,
						Ide->Channels[ j ].Base
					);

					PDISK_OBJECT NewDisk = FsDiskCreateObject( );
					NewDisk->Flags |= DISK_FLAG_CONTROLLER_IDE;
					NewDisk->ControllerData = ExAllocatePoolWithTag( sizeof( IDE_DISK_DATA ), TAGEX_IDE );

					NewDisk->Geometry.Cylinders = Ide->Devices[ Count ].Identity.Cylinders;
					NewDisk->Geometry.Heads = Ide->Devices[ Count ].Identity.Heads;
					NewDisk->Geometry.SectorsPerTrack = Ide->Devices[ Count ].Identity.SectorsPerTrack;
					NewDisk->Geometry.SectorSize = 0x200;

					( ( PIDE_DISK_DATA )NewDisk->ControllerData )->Ide = Ide;
					( ( PIDE_DISK_DATA )NewDisk->ControllerData )->Drive = Count;
					( ( PIDE_DISK_DATA )NewDisk->ControllerData )->Geometry = &NewDisk->Geometry;

					if ( Ide->Devices[ Count ].Type == IDE_ATA ) {

						NewDisk->BootStatus = STATUS_SUCCESS;
						NewDisk->AccessBlock.Read = ( PACCESS_ROUTINE )FsIdeAtaRead;
						NewDisk->AccessBlock.Write = ( PACCESS_ROUTINE )FsIdeAtaWrite;
						NewDisk->Flags |= DISK_FLAG_INTERFACE_ATA;
					}
					else {

						NewDisk->BootStatus = STATUS_UNSUCCESSFUL;
						//NewDisk->AccessBlock.Read = (PDISK_ACCESS)FsIdeAtaRead;
						//NewDisk->AccessBlock.Write = (PDISK_ACCESS)FsIdeAtaWrite;
						NewDisk->Flags |= DISK_FLAG_INTERFACE_ATAPI;
					}

					if ( NT_SUCCESS( NewDisk->BootStatus ) ) {
						NewDisk->BootSector = ExAllocatePoolWithTag( NewDisk->Geometry.SectorSize, TAGEX_BOOT );

						NewDisk->AccessBlock.Read( NewDisk, 0, NewDisk->BootSector, NewDisk->Geometry.SectorSize );

						FsInitializePartitions( NewDisk );
					}

					Count++;
				}
			}


		}
	}
}

VOID
IdeWriteRegister(
	__in PIDE_CHANNEL_REGISTERS Channel,
	__in UCHAR Register,
	__in UCHAR Value
)
{
	//reg > 0x7 then reg -= 0x7 and write to control

	if ( Register == ATA_REG_CONTROL ) {

		__outbyte( Channel->Control, Value );
	}
	else {

		__outbyte( Channel->Base + Register, Value );
	}

	return;
}

UCHAR
IdeReadRegister(
	__in PIDE_CHANNEL_REGISTERS Channel,
	__in UCHAR Register
)
{

	if ( Register == ATA_REG_CONTROL ) {

		return __inbyte( Channel->Control );
	}
	else {

		return __inbyte( Channel->Base + Register );
	}
}

//internal.
UCHAR
IdeAtaAccess(
	__in PIDE_DISK_DATA Disk,
	__in UCHAR Direction,
	__in ULONG64 LogicalBlockAddress,
	__in USHORT Sectors,
	__in UCHAR* Buffer
)
{

#define ATA_FLAG_CHS	0x10
#define ATA_FLAG_LBA28	0x20
#define ATA_FLAG_LBA48	0x40
#define ATA_FLAG_DMA	0x80

	UCHAR Flags = 0, Command = 0;
	UCHAR LogicalBlockAddressIo[ 6 ];
	UCHAR Head = 0;

	UCHAR Channel = Disk->Ide->Devices[ Disk->Drive ].Channel;

	if ( LogicalBlockAddress > Disk->Ide->Devices[ Disk->Drive ].Size ) {


		return ( UCHAR )-1;
	}

	Disk->Ide->Channels[ Channel ].nIEN = ( 1 << 1 );
	Disk->Ide->IdeIrqInvoked = 0;
	IdeWriteRegister( &Disk->Ide->Channels[ Channel ], ATA_REG_CONTROL, ( 1 << 1 ) );

	_memset( LogicalBlockAddressIo, 0, 6 );
	if ( Disk->Ide->Devices[ Disk->Drive ].Identity.CommandSets1 & ( 1 << 26 ) ) {
		Flags |= ATA_FLAG_LBA48;
		LogicalBlockAddressIo[ 0 ] = ( LogicalBlockAddress ) & 0xff;
		LogicalBlockAddressIo[ 1 ] = ( LogicalBlockAddress >> 8 ) & 0xff;
		LogicalBlockAddressIo[ 2 ] = ( LogicalBlockAddress >> 16 ) & 0xff;
		LogicalBlockAddressIo[ 3 ] = ( LogicalBlockAddress >> 24 ) & 0xff;
		LogicalBlockAddressIo[ 4 ] = ( LogicalBlockAddress >> 32 ) & 0xff;
		LogicalBlockAddressIo[ 5 ] = ( LogicalBlockAddress >> 40 ) & 0xff;
	}
	else if ( Disk->Ide->Devices[ Disk->Drive ].Identity.Capabilities & ( 1 << 9 ) ) {
		Flags |= ATA_FLAG_LBA28;
		LogicalBlockAddressIo[ 0 ] = ( LogicalBlockAddress ) & 0xff;
		LogicalBlockAddressIo[ 1 ] = ( LogicalBlockAddress >> 8 ) & 0xff;
		LogicalBlockAddressIo[ 2 ] = ( LogicalBlockAddress >> 16 ) & 0xff;
		Head = ( LogicalBlockAddress >> 24 ) & 0x0f;
	}
	else {
		Flags |= ATA_FLAG_CHS;
		LogicalBlockAddress++;
		/*
			Haha Lua Mode.
		*/

		UCHAR Sector = ( UCHAR )( LogicalBlockAddress % Disk->Geometry->SectorsPerTrack );
		USHORT Cylinder = ( USHORT )( ( LogicalBlockAddress - Sector ) / ( Disk->Geometry->Heads * Disk->Geometry->SectorsPerTrack ) );
		LogicalBlockAddressIo[ 0 ] = Sector;
		LogicalBlockAddressIo[ 1 ] = ( Cylinder ) & 0xff;
		LogicalBlockAddressIo[ 2 ] = ( Cylinder >> 8 ) & 0xff;
		Head = ( UCHAR )( ( LogicalBlockAddress - Sector ) % ( Disk->Geometry->Heads * Disk->Geometry->SectorsPerTrack ) / Disk->Geometry->SectorsPerTrack );
	}

	if ( Direction == DIRECTION_READ ) {
		if ( Flags & ATA_FLAG_DMA ) {
			if ( Flags & ATA_FLAG_CHS )
				Command = ATA_CMD_READ_DMA;
			if ( Flags & ATA_FLAG_LBA28 )
				Command = ATA_CMD_READ_DMA;
			if ( Flags & ATA_FLAG_LBA48 )
				Command = ATA_CMD_READ_DMA_EXT;
		}
		else {
			if ( Flags & ATA_FLAG_CHS )
				Command = ATA_CMD_READ_PIO;
			if ( Flags & ATA_FLAG_LBA28 )
				Command = ATA_CMD_READ_PIO;
			if ( Flags & ATA_FLAG_LBA48 )
				Command = ATA_CMD_READ_PIO_EXT;
		}
	}
	else {
		if ( Flags & ATA_FLAG_DMA ) {
			if ( Flags & ATA_FLAG_CHS )
				Command = ATA_CMD_WRITE_DMA;
			if ( Flags & ATA_FLAG_LBA28 )
				Command = ATA_CMD_WRITE_DMA;
			if ( Flags & ATA_FLAG_LBA48 )
				Command = ATA_CMD_WRITE_DMA_EXT;
		}
		else {
			if ( Flags & ATA_FLAG_CHS )
				Command = ATA_CMD_WRITE_PIO;
			if ( Flags & ATA_FLAG_LBA28 )
				Command = ATA_CMD_WRITE_PIO;
			if ( Flags & ATA_FLAG_LBA48 )
				Command = ATA_CMD_WRITE_PIO_EXT;
		}
	}

	while ( IdeReadRegister( &Disk->Ide->Channels[ Channel ], ATA_REG_STATUS ) & ATA_SR_BSY );

	if ( Flags & ATA_FLAG_CHS )
		IdeWriteRegister( &Disk->Ide->Channels[ Channel ], ATA_REG_HDDEVSEL, 0x80 | 0x20 | ( Disk->Ide->Devices[ Disk->Drive ].Drive << 4 ) | Head );
	else
		IdeWriteRegister( &Disk->Ide->Channels[ Channel ], ATA_REG_HDDEVSEL, 0x40 | 0x80 | 0x20 | ( Disk->Ide->Devices[ Disk->Drive ].Drive << 4 ) | Head );

	if ( Flags & ATA_FLAG_LBA48 ) {
		/*
			should be written first ?
		*/
		IdeWriteRegister( &Disk->Ide->Channels[ Channel ], ATA_REG_SECCOUNT0, ( Sectors >> 8 ) & 0xff );
		IdeWriteRegister( &Disk->Ide->Channels[ Channel ], ATA_REG_LBA0, LogicalBlockAddressIo[ 3 ] );
		IdeWriteRegister( &Disk->Ide->Channels[ Channel ], ATA_REG_LBA1, LogicalBlockAddressIo[ 4 ] );
		IdeWriteRegister( &Disk->Ide->Channels[ Channel ], ATA_REG_LBA2, LogicalBlockAddressIo[ 5 ] );
	}

	IdeWriteRegister( &Disk->Ide->Channels[ Channel ], ATA_REG_SECCOUNT0, Sectors & 0xff );
	IdeWriteRegister( &Disk->Ide->Channels[ Channel ], ATA_REG_LBA0, LogicalBlockAddressIo[ 0 ] );
	IdeWriteRegister( &Disk->Ide->Channels[ Channel ], ATA_REG_LBA1, LogicalBlockAddressIo[ 1 ] );
	IdeWriteRegister( &Disk->Ide->Channels[ Channel ], ATA_REG_LBA2, LogicalBlockAddressIo[ 2 ] );

	IdeWriteRegister( &Disk->Ide->Channels[ Channel ], ATA_REG_COMMAND, Command );

	if ( Flags & ATA_FLAG_DMA ) {
		if ( Direction == DIRECTION_READ ) {

		}
		else {

		}
	}
	else {
		if ( Direction == DIRECTION_READ ) {

			for ( USHORT i = 0; i < Sectors; i++ ) {
				//IdeDelay( &Disk->Ide->Channels[ Channel ] );

				while ( IdeReadRegister( &Disk->Ide->Channels[ Channel ], ATA_REG_STATUS ) & ATA_SR_BSY );

				UCHAR Status = IdeReadRegister( &Disk->Ide->Channels[ Channel ], ATA_REG_STATUS );

				if ( Status & ATA_SR_ERR ) {

					return IdeReadRegister( &Disk->Ide->Channels[ Channel ], ATA_REG_ERROR );
				}

				if ( Status & ATA_SR_DF ) {

					return Status;
				}

				if ( ( Status & ATA_SR_DRQ ) == 0 ) {

					return Status;
				}

				for ( USHORT j = 0; j < ( Disk->Geometry->SectorSize / 2 ); j++ )
					( ( USHORT* )Buffer )[ j ] = __inword( Disk->Ide->Channels[ Channel ].Base + ATA_REG_DATA );
				Buffer += ( Disk->Geometry->SectorSize );
			}
		}
		else {

		}
	}

#undef ATA_FLAG_LBA48
#undef ATA_FLAG_LBA24
#undef ATA_FLAG_CHS

	return 0;
}

NTSTATUS
FsIdeAtaRead(
	__in PDISK_OBJECT Disk,
	__in ULONG64 LogicalBlockAddress,
	__in PVOID Buffer,
	__in ULONG Length
)
{

	if ( Length % Disk->Geometry.SectorSize != 0 ) {

		return STATUS_INVALID_PARAMETER;
	}

	Length /= Disk->Geometry.SectorSize;
	ULONG SectorOffset = 0;
	UCHAR* BufferPtr = Buffer;

	for ( ; Length > 0xFFFF; Length -= 0xFFFF, SectorOffset += 0xFFFF, BufferPtr += ( 0xFFFF * Disk->Geometry.SectorSize ) ) {
		UCHAR Error = IdeAtaAccess( ( PIDE_DISK_DATA )Disk->ControllerData, DIRECTION_READ, LogicalBlockAddress + SectorOffset, 0xFFFF, BufferPtr );

		if ( Error != 0 ) {

			if ( SectorOffset != 0 )
				return STATUS_PARTIAL_COMPLETE;
			else
				return STATUS_UNSUCCESSFUL;

		}
	}

	if ( IdeAtaAccess( ( PIDE_DISK_DATA )Disk->ControllerData, DIRECTION_READ, LogicalBlockAddress + SectorOffset, ( USHORT )Length, BufferPtr ) != 0 ) {

		if ( SectorOffset != 0 )
			return STATUS_PARTIAL_COMPLETE;
		else
			return STATUS_UNSUCCESSFUL;
	}

	return STATUS_SUCCESS;
}

NTSTATUS
FsIdeAtaWrite(
	__in PDISK_OBJECT Disk,
	__in ULONG64 LogicalBlockAddress,
	__in PVOID Buffer,
	__in ULONG Length
)
{

	if ( Length % Disk->Geometry.SectorSize != 0 ) {

		return STATUS_INVALID_PARAMETER;
	}

	Length /= Disk->Geometry.SectorSize;
	ULONG SectorOffset = 0;
	UCHAR* BufferPtr = Buffer;

	for ( ; Length > 0xFFFF; Length -= 0xFFFF, SectorOffset += 0xFFFF, BufferPtr += ( 0xFFFF * Disk->Geometry.SectorSize ) ) {
		UCHAR Error = IdeAtaAccess( ( PIDE_DISK_DATA )Disk->ControllerData, DIRECTION_WRITE, LogicalBlockAddress + SectorOffset, 0xFFFF, BufferPtr );

		if ( Error != 0 ) {

			if ( SectorOffset != 0 )
				return STATUS_PARTIAL_COMPLETE;
			else
				return STATUS_UNSUCCESSFUL;

		}
	}

	if ( IdeAtaAccess( ( PIDE_DISK_DATA )Disk->ControllerData, DIRECTION_WRITE, LogicalBlockAddress + SectorOffset, ( USHORT )Length, BufferPtr ) != 0 ) {

		if ( SectorOffset != 0 )
			return STATUS_PARTIAL_COMPLETE;
		else
			return STATUS_UNSUCCESSFUL;
	}

	return STATUS_SUCCESS;
}

