


#include "driver.h"
#include "controller.h"
#include "ahci.h"

VOID
AhciStartCommandEngine(
	__in PHBA_PORT Port
);

VOID
AhciStopCommandEngine(
	__in PHBA_PORT Port
);

VOID
AhciPortRebase(
	__in PAHCI_DISK_DATA Disk
);

ULONG32
AhciFindFreeSlot(
	__in PAHCI_DISK_DATA Disk
);

UCHAR
AhciSendCommand(
	__in PHBA_PORT Port,
	__in ULONG Slot
);

NTSTATUS
AhciAtaAccess(
	__in PAHCI_DISK_DATA Disk,
	__in UCHAR Direction,
	__in ULONG64 LogicalBlockAddress,
	__in USHORT Sectors,
	__in PVOID Buffer
);

NTSTATUS
FsAhciAtaWrite(
	__in PDISK_OBJECT Disk,
	__in ULONG64 LogicalBlockAddress,
	__in PVOID Buffer,
	__in ULONG Length
);

NTSTATUS
FsAhciAtaRead(
	__in PDISK_OBJECT Disk,
	__in ULONG64 LogicalBlockAddress,
	__in PVOID Buffer,
	__in ULONG Length
);

VOID
FsAhciDetectDrives(

)
{

	for ( ULONG32 i = 0; i < HalPciDeviceList.DeviceCount; i++ ) {

		/*
			0x6 is Serial ATA

			0x1 is AHCI Prog_IF
		*/

		/*
			Class
			0x06 - Serial ATA

			ProgIf
			0x00 - Vendor Specific Interface
			0x01 - AHCI 1.0
			0x02 - Serial Storage Bus
		*/

		if ( HalPciDeviceList.PciDevices[ i ].PciHeader.ClassCode == PCI_CLASS_CODE_MASS_STORAGE_CONTROLLER &&
			HalPciDeviceList.PciDevices[ i ].PciHeader.SubClass == 0x6 &&
			HalPciDeviceList.PciDevices[ i ].PciHeader.Prog_IF == 0x1 ) {

			//NTSTATUS ntStatus;
			PAHCI_CONTROLLER Ahci = ExAllocatePoolWithTag( sizeof( AHCI_CONTROLLER ), TAGEX_AHCI );

			Ahci->PciDevice = &HalPciDeviceList.PciDevices[ i ];

			//HalPciWrite32(Ahci->PciDevice, 0x4, HalPciRead32(Ahci->PciDevice, 0x4) | PCI_COMMAND_BUS_MASTER);

			/*
				We only need bar5, which is the ahci mmio,
				the other 5 are for ide legacy mode.
			*/
			PCI_BASE_ADDRESS_REGISTER Bar5;

			HalPciReadBar( Ahci->PciDevice, &Bar5, 5 );

			Ahci->Physical = Bar5.Base;
			Ahci->Io = MmAllocateMemoryAtPhysical( Ahci->Physical, sizeof( HBA_MMIO ), PAGE_READ | PAGE_WRITE );

			/*
			Ahci->Physical = Bar5.Base;
			Ahci->Io = (PHBA_MMIO)MmpFindFreeVirtual(sizeof(HBA_MMIO), PAGE_READ | PAGE_WRITE);
			ntStatus = MmAllocateMemoryAtPhysical(Ahci->Physical, (ULONG64)Ahci->Io, sizeof(HBA_MMIO), PAGE_READ | PAGE_WRITE);

			if (!NT_SUCCESS(ntStatus)) {

				ExFreePoolWithTag(Ahci, 'ichA');
				continue;
			}*/

			for ( ULONG32 j = 0; j < 32; j++ ) {

				if ( ( Ahci->Io->PortImplemented >> j ) & 1 ) {

					ULONG DeviceType = AHCI_DEV_NULL;
					ULONG Status = Ahci->Io->Ports[ j ].SataStatus;

					if ( ( Status & 0x0F ) != HBA_PORT_DET_PRESENT )
						continue;

					if ( ( ( Status >> 8 ) & 0x0F ) != HBA_PORT_IPM_ACTIVE )
						continue;

					switch ( Ahci->Io->Ports[ j ].Signature ) {
					case SATA_SIG_ATAPI:
						DeviceType = AHCI_DEV_ATAPI;
						break;
					case SATA_SIG_SEMB:
						DeviceType = AHCI_DEV_SEMB;
						break;
					case SATA_SIG_PM:
						DeviceType = AHCI_DEV_PM;
						break;
					case SATA_SIG_ATA:
					default:
						DeviceType = AHCI_DEV_ATA;
						break;
					}

					if ( DeviceType != AHCI_DEV_ATA && DeviceType != AHCI_DEV_ATAPI )
						continue;

					PDISK_OBJECT NewDisk = FsDiskCreateObject( );
					NewDisk->Flags |= DISK_FLAG_CONTROLLER_AHCI;
					NewDisk->ControllerData = ExAllocatePoolWithTag( sizeof( AHCI_DISK_DATA ), TAGEX_AHCI );

					PAHCI_DISK_DATA ControllerData = ( ( PAHCI_DISK_DATA )NewDisk->ControllerData );

					ControllerData->Ahci = Ahci;
					ControllerData->Port = &Ahci->Io->Ports[ j ];

					AhciPortRebase( ControllerData );

					ULONG32 Slot = AhciFindFreeSlot( ControllerData );

					PHBA_COMMAND_HEADER CommandHeader = &ControllerData->CommandList[ Slot ];
					CommandHeader->FisLength = sizeof( FIS_REG_H2D ) / sizeof( ULONG32 );
					CommandHeader->Write = 0;
					CommandHeader->PrdtLength = 1;

					PHBA_COMMAND_TABLE CommandTable = &ControllerData->CommandTable[ Slot * 2 ];
					_memset( ( void* )CommandTable, 0, sizeof( HBA_COMMAND_TABLE ) + ( CommandHeader->PrdtLength * sizeof( HBA_PRDT_ENTRY ) ) );

					PATA_DEVICE_IDENTITY IdentityBuffer = MmAllocateMemory( sizeof( ATA_DEVICE_IDENTITY ), PAGE_READ | PAGE_WRITE );

					CommandTable->PrdtEntries[ 0 ].DataBaseAddress = MmPhysicalMapping( ( ULONG64 )IdentityBuffer );
					CommandTable->PrdtEntries[ 0 ].ByteCount = 511;
					CommandTable->PrdtEntries[ 0 ].Interrupt = 1;

					PFIS_REG_H2D Fis = ( PFIS_REG_H2D )&CommandTable->CommandFis;

					_memset( ( void* )Fis, 0, sizeof( FIS_REG_H2D ) );
					Fis->Type = FIS_TYPE_REG_H2D;
					Fis->CommandRegister = ATA_CMD_IDENTIFY;
					Fis->DeviceRegister = 0;
					Fis->PmPort = 0;
					Fis->Command = 1;

					if ( AhciSendCommand( ControllerData->Port, Slot ) != 0 ) {

						DbgPrint( L"identify failed.\n" );
					}

					_memcpy( &ControllerData->Identity, IdentityBuffer, sizeof( ATA_DEVICE_IDENTITY ) );

					ControllerData->Size = ControllerData->Identity.MaxLogicalBlockAddressExt;

					for ( ULONG l = 0; l < 40; l += 2 ) {
						UCHAR Temp = ControllerData->Identity.ModelNumber[ l ];
						ControllerData->Identity.ModelNumber[ l ] = ControllerData->Identity.ModelNumber[ l + 1 ];
						ControllerData->Identity.ModelNumber[ l + 1 ] = Temp;
					}
					ControllerData->Identity.ModelNumber[ 39 ] = 0;

					DbgPrint( L"drive model: %a\nsectors: %d, cylinders: %d, heads: %d, spt: %d\n",
						ControllerData->Identity.ModelNumber,
						ControllerData->Size,
						ControllerData->Identity.Cylinders,
						ControllerData->Identity.Heads,
						ControllerData->Identity.SectorsPerTrack
					);

					NewDisk->Geometry.Cylinders = ControllerData->Identity.Cylinders;
					NewDisk->Geometry.Heads = ControllerData->Identity.Heads;
					NewDisk->Geometry.SectorsPerTrack = ControllerData->Identity.SectorsPerTrack;
					NewDisk->Geometry.SectorSize = 0x200;

					ControllerData->Geometry = &NewDisk->Geometry;
					ControllerData->Type = DeviceType;

					if ( ControllerData->Type == AHCI_DEV_ATA ) {

						NewDisk->BootStatus = STATUS_SUCCESS;
						NewDisk->AccessBlock.Read = ( PACCESS_ROUTINE )FsAhciAtaRead;
						NewDisk->AccessBlock.Write = ( PACCESS_ROUTINE )FsAhciAtaWrite;
						NewDisk->Flags |= DISK_FLAG_INTERFACE_ATA;
					}
					else {

						NewDisk->BootStatus = STATUS_UNSUCCESSFUL;

						NewDisk->Flags |= DISK_FLAG_INTERFACE_ATAPI;
					}

					if ( NT_SUCCESS( NewDisk->BootStatus ) ) {
						NewDisk->BootSector = ExAllocatePoolWithTag( NewDisk->Geometry.SectorSize, TAGEX_BOOT );

						NewDisk->AccessBlock.Read( NewDisk, 0, NewDisk->BootSector, NewDisk->Geometry.SectorSize );

						FsInitializePartitions( NewDisk );
					}

				}

			}

		}
	}
}

VOID
AhciPortRebase(
	__in PAHCI_DISK_DATA Disk
)
{
	AhciStopCommandEngine( Disk->Port );

	Disk->CommandList = MmAllocateContiguousMemory( sizeof( PHBA_COMMAND_HEADER[ 32 ] ), PAGE_READ | PAGE_WRITE );
	Disk->Port->CommandListBase = MmPhysicalMapping( ( ULONG64 )Disk->CommandList );

	_memset( ( void* )Disk->CommandList, 0, sizeof( HBA_COMMAND_HEADER[ 32 ] ) );

	Disk->Fis = MmAllocateContiguousMemory( sizeof( HBA_FIS ), PAGE_READ | PAGE_WRITE );
	Disk->Port->FisBase = MmPhysicalMapping( ( ULONG64 )Disk->Fis );

	_memset( ( void* )Disk->Fis, 0, sizeof( HBA_FIS ) );

	Disk->CommandTable = MmAllocateContiguousMemory( sizeof( HBA_COMMAND_TABLE[ 32 ] ), PAGE_READ | PAGE_WRITE );
	Disk->CommandTablePhysical = MmPhysicalMapping( ( ULONG64 )Disk->CommandTable );
	_memset( ( void* )Disk->CommandTable, 0, sizeof( HBA_COMMAND_TABLE[ 32 ] ) );

	for ( ULONG i = 0; i < 32; i++ ) {

		Disk->CommandList[ i ].PrdtLength = 1;
		Disk->CommandList[ i ].CommandTableDescriptorBase = Disk->CommandTablePhysical + i * sizeof( HBA_COMMAND_TABLE );
	}

	//AhciStartCommandEngine(Disk->Port);
}

VOID
AhciStartCommandEngine(
	__in PHBA_PORT Port
)
{
	Port->CommandStatus &= ~HBA_PxCMD_ST;
	while ( Port->CommandStatus & HBA_PxCMD_CR );
	Port->CommandStatus |= ( HBA_PxCMD_FRE | HBA_PxCMD_ST );

}

VOID
AhciStopCommandEngine(
	__in PHBA_PORT Port
)
{
	Port->CommandStatus &= ~HBA_PxCMD_ST;
	while ( Port->CommandStatus & HBA_PxCMD_CR );
	Port->CommandStatus &= ~HBA_PxCMD_FRE;
}

ULONG32
AhciFindFreeSlot(
	__in PAHCI_DISK_DATA Disk
)
{

	ULONG32 Slots = ( Disk->Port->SataActive | Disk->Port->CommandIssue );

	for ( ULONG32 i = 0; i < 32; i++ ) {

		if ( ( ( Slots >> i ) & 1 ) == 0 )
			return i;
	}

	return ( ULONG32 )-1;
}

UCHAR
AhciSendCommand(
	__in PHBA_PORT Port,
	__in ULONG Slot
)
{
	AhciStartCommandEngine( Port );

	//while (Port->TaskFileData & (ATA_SR_BSY | ATA_SR_DRQ));

	Port->CommandIssue = ( 1 << Slot );

	while ( 1 ) {

		if ( ( Port->CommandIssue & ( 1 << Slot ) ) == 0 )
			break;

		if ( Port->InterruptStatus & HBA_PxIS_TFES )
			return ( UCHAR )-1;
	}

	if ( Port->InterruptStatus & HBA_PxIS_TFES )
		return ( UCHAR )-1;

	AhciStopCommandEngine( Port );

	return 0;
}

NTSTATUS
AhciAtaAccess(
	__in PAHCI_DISK_DATA Disk,
	__in UCHAR Direction,
	__in ULONG64 LogicalBlockAddress,
	__in USHORT Sectors,
	__in PVOID Buffer
)
{
	//NTSTATUS ntStatus;

	Disk->Port->InterruptStatus = ( ULONG32 )-1;

	ULONG64 BufferSize = Sectors * 512;

	ULONG64 VirtualBufferAddress = ( ULONG64 )MmAllocateContiguousMemory( BufferSize, PAGE_READ | PAGE_WRITE );
	ULONG64 PhysicalBufferAddress = MmPhysicalMapping( VirtualBufferAddress );
	//ULONG64 PhysicalBufferAddress = MmpFindFreePhysicalRange( BufferSize );
	//ULONG64 VirtualBufferAddress = MmpFindFreeVirtual( BufferSize, PAGE_READ | PAGE_WRITE );
	//ntStatus = MmAllocateMemoryAtPhysical( PhysicalBufferAddress, VirtualBufferAddress, BufferSize, PAGE_READ | PAGE_WRITE );

	//if ( !NT_SUCCESS( ntStatus ) ) {

	//	return ntStatus;
	//}

	ULONG32 Slot = AhciFindFreeSlot( Disk );

	if ( Slot == ( ULONG32 )-1 ) {

		return STATUS_DEVICE_FAILED;
	}

	PHBA_COMMAND_HEADER CommandHeader = &Disk->CommandList[ Slot ];
	//for (ULONG64 i = 0; i < (sizeof(HBA_COMMAND_HEADER) / sizeof(ULONG32)); i++)
	//	*((ULONG32*)CommandHeader + i) = 0;

	CommandHeader->FisLength = sizeof( FIS_REG_H2D ) / sizeof( ULONG32 );
	CommandHeader->Write = Direction;
	CommandHeader->PrdtLength = 1;

	PHBA_COMMAND_TABLE CommandTable = &Disk->CommandTable[ Slot ];
	//for (ULONG64 i = 0; i < (sizeof(HBA_COMMAND_TABLE) / sizeof(ULONG32)); i++)
	//	*((ULONG32*)CommandTable + i) = 0;

	CommandTable->PrdtEntries[ 0 ].DataBaseAddress = PhysicalBufferAddress;
	CommandTable->PrdtEntries[ 0 ].ByteCount = ( ULONG32 )BufferSize - 1;
	CommandTable->PrdtEntries[ 0 ].Interrupt = 1;

	PFIS_REG_H2D Fis = ( PFIS_REG_H2D )&CommandTable->CommandFis;

	Fis->Type = FIS_TYPE_REG_H2D;
	Fis->Command = 1;
	Fis->CommandRegister = ATA_CMD_READ_DMA_EXT;

	Fis->LogicalBlockAddress0 = ( LogicalBlockAddress ) & 0xff;
	Fis->LogicalBlockAddress1 = ( LogicalBlockAddress >> 8 ) & 0xff;
	Fis->LogicalBlockAddress2 = ( LogicalBlockAddress >> 16 ) & 0xff;
	Fis->DeviceRegister = 1 << 6; //lba mode.

	Fis->LogicalBlockAddress3 = ( LogicalBlockAddress >> 24 ) & 0xff;
	Fis->LogicalBlockAddress4 = ( LogicalBlockAddress >> 32 ) & 0xff;
	Fis->LogicalBlockAddress5 = ( LogicalBlockAddress >> 40 ) & 0xff;

	Fis->Count = Sectors;

	if ( AhciSendCommand( Disk->Port, Slot ) != 0 ) {

		return STATUS_DEVICE_FAILED;
	}

	CommandTable->PrdtEntries[ 0 ].DataBaseAddress = 0;
	CommandTable->PrdtEntries[ 0 ].ByteCount = 0;
	CommandTable->PrdtEntries[ 0 ].Interrupt = 0;

	for ( ULONG64 i = 0; i < ( BufferSize / sizeof( ULONG64 ) ); i++ )
		( ( ULONG64* )Buffer )[ i ] = ( ( ULONG64* )VirtualBufferAddress )[ i ];

	//_memcpy((void*)Buffer, (void*)VirtualBufferAddress, (int)BufferSize);
	MmFreeMemory( VirtualBufferAddress, BufferSize );

	return STATUS_SUCCESS;
}

NTSTATUS
FsAhciAtaRead(
	__in PDISK_OBJECT Disk,
	__in ULONG64 LogicalBlockAddress,
	__in PVOID Buffer,
	__in ULONG Length
)
{
	NTSTATUS ntStatus;

	if ( Length % Disk->Geometry.SectorSize != 0 ) {

		return STATUS_INVALID_PARAMETER;
	}

	Length /= Disk->Geometry.SectorSize;
	ULONG64 SectorOffset = 0;
	UCHAR* BufferPtr = Buffer;

	for ( ; Length > 128; Length -= 128, SectorOffset += 128, BufferPtr += ( 128 * Disk->Geometry.SectorSize ) ) {
		ntStatus = AhciAtaAccess( ( PAHCI_DISK_DATA )Disk->ControllerData, DIRECTION_READ, LogicalBlockAddress + SectorOffset, 128, BufferPtr );

		if ( !NT_SUCCESS( ntStatus ) ) {

			if ( SectorOffset != 0 )
				return STATUS_PARTIAL_COMPLETE;
			else
				return ntStatus;

		}
	}

	ntStatus = AhciAtaAccess( ( PAHCI_DISK_DATA )Disk->ControllerData, DIRECTION_READ, LogicalBlockAddress + SectorOffset, ( USHORT )Length, BufferPtr );
	if ( !NT_SUCCESS( ntStatus ) ) {

		if ( SectorOffset != 0 )
			return STATUS_PARTIAL_COMPLETE;
		else
			return ntStatus;
	}

	return STATUS_SUCCESS;
}

NTSTATUS
FsAhciAtaWrite(
	__in PDISK_OBJECT Disk,
	__in ULONG64 LogicalBlockAddress,
	__in PVOID Buffer,
	__in ULONG Length
)
{
	NTSTATUS ntStatus;

	if ( Length % Disk->Geometry.SectorSize != 0 ) {

		return STATUS_INVALID_PARAMETER;
	}

	Length /= Disk->Geometry.SectorSize;
	ULONG SectorOffset = 0;
	UCHAR* BufferPtr = Buffer;

	for ( ; Length > 128; Length -= 128, SectorOffset += 128, BufferPtr += ( 128 * Disk->Geometry.SectorSize ) ) {
		ntStatus = AhciAtaAccess( ( PAHCI_DISK_DATA )Disk->ControllerData, DIRECTION_WRITE, LogicalBlockAddress + SectorOffset, 128, BufferPtr );

		if ( !NT_SUCCESS( ntStatus ) ) {

			if ( SectorOffset != 0 )
				return STATUS_PARTIAL_COMPLETE;
			else
				return ntStatus;

		}
	}

	ntStatus = AhciAtaAccess( ( PAHCI_DISK_DATA )Disk->ControllerData, DIRECTION_WRITE, LogicalBlockAddress + SectorOffset, ( USHORT )Length, BufferPtr );

	if ( !NT_SUCCESS( ntStatus ) ) {

		if ( SectorOffset != 0 )
			return STATUS_PARTIAL_COMPLETE;
		else
			return ntStatus;
	}

	return STATUS_SUCCESS;
}
