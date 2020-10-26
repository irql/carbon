

#include "driver.h"

NTSTATUS
FsPartitionRead(
	__in PPARTITION_OBJECT Partition,
	__in ULONG64 LogicalBlockAddress,
	__in PVOID Buffer,
	__in ULONG Length
	)
{

	LogicalBlockAddress += Partition->StartLogicalBlockAddress;

	return Partition->Disk->AccessBlock.Read(Partition->Disk, LogicalBlockAddress, Buffer, Length);
}

NTSTATUS
FsPartitionWrite(
	__in PPARTITION_OBJECT Partition,
	__in ULONG64 LogicalBlockAddress,
	__in PVOID Buffer,
	__in ULONG Length
	)
{

	LogicalBlockAddress += Partition->StartLogicalBlockAddress;

	return Partition->Disk->AccessBlock.Read(Partition->Disk, LogicalBlockAddress, Buffer, Length);
}

PPARTITION_OBJECT
FsPartitionCreateObject(
	__in PDISK_OBJECT Disk
	)
{
	PPARTITION_OBJECT New = (PPARTITION_OBJECT)ExAllocatePoolWithTag(sizeof(PARTITION_OBJECT), TAGEX_PARTITION);
	_memset((void*)New, 0, sizeof(PARTITION_OBJECT));

	New->Disk = Disk;
	New->AccessBlock.Read = (PACCESS_ROUTINE)FsPartitionRead;
	New->AccessBlock.Write = (PACCESS_ROUTINE)FsPartitionWrite;

	return New;
}

VOID
FsInitializePartitions(
	__in PDISK_OBJECT Disk
	)
{

	//
	//	how do you detect mbr partitioning?
	//
	
	Disk->Flags |= DISK_FLAG_PARTITION_STYLE_MBR;

	PMBR_BOOT_SECTOR MasterBootRecord = Disk->BootSector;

	ULONG ActivePartition = (unsigned)-1;

	PPARTITION_OBJECT FirstPartition = FsPartitionCreateObject(Disk);
	KeInitializeListHead(&FirstPartition->PartitionLinks);
	Disk->PartitionLinks = &FirstPartition->PartitionLinks;

	FirstPartition->Identifier = Disk->PartitionCount++;

	for (ULONG i = 0; i < 4; i++) {

		if (MasterBootRecord->PartitionTable[i].StartLogicalBlockAddress == 0 ||
			MasterBootRecord->PartitionTable[i].SectorCount == 0)
			continue;

		if (MasterBootRecord->PartitionTable[i].Attributes & MBR_ATTRIBUTE_ACTIVE) {

			ActivePartition = i;
			break;
		}
	}

	for (ULONG i = 0; i < 4; i++) {

		if (MasterBootRecord->PartitionTable[i].StartLogicalBlockAddress == 0 ||
			MasterBootRecord->PartitionTable[i].SectorCount == 0)
			continue;

		if (ActivePartition == i || ActivePartition == -1) {

			FirstPartition->SectorCount = MasterBootRecord->PartitionTable[i].SectorCount;
			FirstPartition->StartLogicalBlockAddress = MasterBootRecord->PartitionTable[i].StartLogicalBlockAddress;

			if (MasterBootRecord->PartitionTable[i].Attributes & MBR_ATTRIBUTE_ACTIVE) {

				FirstPartition->Flags |= PARTITION_FLAG_ACTIVE;
			}

			FirstPartition->BootSector = ExAllocatePoolWithTag(Disk->Geometry.SectorSize, TAGEX_BOOT);
			FirstPartition->BootStatus = FirstPartition->AccessBlock.Read(FirstPartition, 0, FirstPartition->BootSector, Disk->Geometry.SectorSize);

			if (!NT_SUCCESS(FirstPartition->BootStatus)) {

				ExFreePoolWithTag(FirstPartition->BootSector, 'tooB');
				FirstPartition->BootSector = NULL;
			}
			else {

				

				FirstPartition->Flags |= PARTITION_FLAG_FILE_SYSTEM_FAT32;
				FsFat32InitializePartition(FirstPartition);
				
			}

			ActivePartition = i;
		}
		else {

			PPARTITION_OBJECT NewPartition = FsPartitionCreateObject(Disk);
			KeInsertListEntry(&FirstPartition->PartitionLinks, &NewPartition->PartitionLinks);

			NewPartition->Identifier = Disk->PartitionCount++;
			NewPartition->SectorCount = MasterBootRecord->PartitionTable[i].SectorCount;
			NewPartition->StartLogicalBlockAddress = MasterBootRecord->PartitionTable[i].StartLogicalBlockAddress;

			if (MasterBootRecord->PartitionTable[i].Attributes & MBR_ATTRIBUTE_ACTIVE) {

				NewPartition->Flags |= PARTITION_FLAG_ACTIVE;
			}

			NewPartition->BootSector = ExAllocatePoolWithTag(Disk->Geometry.SectorSize, TAGEX_BOOT);
			NewPartition->BootStatus = Disk->AccessBlock.Read(Disk, NewPartition->StartLogicalBlockAddress, NewPartition->BootSector, Disk->Geometry.SectorSize);

			if (!NT_SUCCESS(NewPartition->BootStatus)) {

				ExFreePoolWithTag(NewPartition->BootSector, 'tooB');
				NewPartition->BootSector = NULL;
			}
			else {

				

				NewPartition->Flags |= PARTITION_FLAG_FILE_SYSTEM_FAT32;
				FsFat32InitializePartition(NewPartition);
				
			}
		}

		PPARTITION_OBJECT temp = FsGetPartitionById(Disk, Disk->PartitionCount - 1);
		DbgPrint(L"partition%d detected. start: %d, size: %d\n", temp->Identifier, temp->StartLogicalBlockAddress, temp->SectorCount);

	}


	return;
}
