/*++

Module ObjectName:

	fat32.h

Abstract:

	Defines internal fat32 procedures and data structures.

--*/

#pragma once

#include "driver.h"

#define FAT32_READ_ONLY				0x01
#define FAT32_HIDDEN				0x02
#define FAT32_SYSTEM				0x04
#define FAT32_VOLUME_ID				0x08
#define FAT32_DIRECTORY				0x10
#define FAT32_ARCHIVE				0x20
#define FAT32_LFN					(FAT32_READ_ONLY | FAT32_HIDDEN | FAT32_SYSTEM | FAT32_VOLUME_ID)

#define FAT32_END_OF_CHAIN			0x0FFFFFFF
#define FAT32_DIRECTORY_ENTRY_FREE	0xE5
#define FAT32_LAST_LFN_ENTRY		0x40

typedef struct _FAT32_DIRECTORY_ENTRY {
	CHAR		Name[8];
	CHAR		Extension[3];
	UCHAR		Attributes;
	UCHAR		Reserved;
	UCHAR		CreateTimeTenth;
	USHORT		CreateTime;
	USHORT		CreateDate;
	USHORT		AccessDate;
	USHORT		ClusterHigh;
	USHORT		ModifiedTime;
	USHORT		ModifiedDate;
	USHORT		ClusterLow;
	ULONG		FileSize;
} FAT32_DIRECTORY_ENTRY, *PFAT32_DIRECTORY_ENTRY;

typedef struct _FAT32_LFN_DIRECTORY_ENTRY {
	UCHAR		OrderOfEntry;
	WCHAR		First5Chars[5];
	UCHAR		Attributes;
	UCHAR		LongEntryType;
	UCHAR		NameChecksum;
	WCHAR		Next6Chars[6];
	USHORT		Zero;
	WCHAR		Next2Chars[2];
} FAT32_LFN_DIRECTORY_ENTRY, *PFAT32_LFN_DIRECTORY_ENTRY;

typedef struct _DOS200BPB {
	CHAR		FileSystemIdentifier[8];
	USHORT		BytesPerSector;
	UCHAR		SectorsPerCluster;
	USHORT		ReservedSectors;
	UCHAR		FatCount;
	USHORT		RootDirectoryEntriesCount;
	USHORT		TotalSectors16;
	UCHAR		MediaDesciptor;
	USHORT		SectorsPerFat;
} DOS200BPB, *PDOS200BPB;

typedef struct _DOS331BPB {
	USHORT		SectorsPerTrack;
	USHORT		NumberOfHeads;
	ULONG		HiddenSectors;
	ULONG		TotalSectors32;
} DOS331BPB, *PDOS331BPB;

typedef struct _DOS701BPB {
	ULONG		SectorsPerFat;
	USHORT		MirroringFlags;
	USHORT		FatVersion;
	ULONG		RootDirectoryCluster;
	USHORT		FileSystemInfoSector;
	USHORT		BackupSectors;
	UCHAR		Reserved[12];
	UCHAR		BootDisk;
	UCHAR		NtFlags;
	UCHAR		ExtendedBootSignature;
	ULONG		VolumeSerialNumber;
	UCHAR		VolumeLabel[11];
	UCHAR		SystemIdentifierString[8];
} DOS701BPB, *PDOS701BPB;

typedef struct _BIOS_PARAMETER_BLOCK {
	CHAR		Jump[3];
	DOS200BPB	Dos2_00Bpb;
	DOS331BPB	Dos3_31Bpb;
	DOS701BPB	Dos7_01Bpb;		//EBPB
	UCHAR		BootCode[420];
	USHORT		BootSignature;
} BIOS_PARAMETER_BLOCK, *PBIOS_PARAMETER_BLOCK;

typedef struct _FAT32_FILE_SYSTEM_INFO {
	ULONG		Signature;
	UCHAR		Reserved[480];
	ULONG		Signature2;
	ULONG		LastKnownFreeClusterCount;
	ULONG		LastKnownFreeCluster;
	UCHAR		Reserved1[12];
	ULONG		Signature3;
} FAT32_FILE_SYSTEM_INFO, *PFAT32_FILE_SYSTEM_INFO;

#define FIRST_DATA_SECTOR(b)			(b->Dos2_00Bpb.ReservedSectors + (b->Dos2_00Bpb.FatCount * b->Dos7_01Bpb.SectorsPerFat))
#define FIRST_SECTOR_OF_CLUSTER(b, n)	((((n) - 2) * b->Dos2_00Bpb.SectorsPerCluster) + FIRST_DATA_SECTOR(b))

typedef struct _FAT32FS {
	PVOID Access;

	PACCESS_BLOCK Drive;
	PBIOS_PARAMETER_BLOCK Bpb;

} FAT32FS, *PFAT32FS;

typedef enum _FAT32PATHTYPE {
	Fat32PathInvalid,
	Fat32Path8Dot3,
	Fat32PathLongFileName

} FAT32PATHTYPE;

typedef UCHAR FAT32_PATH_TYPE;

FAT32_PATH_TYPE
FsFat32VerifyFileName(
	__in PWCHAR FileName
	);

VOID
FsFat32ConvertPathTo8Dot3(
	__in PWCHAR FileName,
	__out PCHAR FileName8Dot3
	);

NTSTATUS
FsFat32ReadDirectoryFile(
	__in PPARTITION_OBJECT Partition,
	__in PFILE_OBJECT FileObject,
	__in PIO_STATUS_BLOCK UserIosb
	);

ULONG32
FspFat32QueryFatTable(
	__in PFAT32FS Drive,
	__in ULONG32 Cluster
	);

ULONG64
FsFat32FindFile(
	__in PFAT32_DIRECTORY_ENTRY Directory,
	__in FAT32_PATH_TYPE PathType,
	__in PWCHAR FileName
	);

NTSTATUS
FsFat32WriteDirectoryFile(
	__in PPARTITION_OBJECT Partition,
	__in PFILE_OBJECT FileObject,
	__in PIO_STATUS_BLOCK UserIosb
	);