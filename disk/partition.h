


#pragma once


#define MBR_ATTRIBUTE_ACTIVE 0x80

typedef struct _MBR_PARTITION_TABLE_ENTRY {
	UCHAR		Attributes;
	UCHAR		StartCylinderHeadSector[3];
	UCHAR		Type;
	UCHAR		EndCylinderHeadSector[3];
	ULONG		StartLogicalBlockAddress;
	ULONG		SectorCount;
} MBR_PARTITION_TABLE_ENTRY, *PMBR_PARTITION_TABLE_ENTRY;

typedef struct _MBR_BOOT_SECTOR {
	UCHAR						BootCode[440];
	ULONG						UniqueDiskId;
	USHORT						Reserved;
	MBR_PARTITION_TABLE_ENTRY	PartitionTable[4];
	USHORT						BootSignature;
} MBR_BOOT_SECTOR, *PMBR_BOOT_SECTOR;

VOID
FsInitializePartitions(
	__in PDISK_OBJECT Disk
	);
