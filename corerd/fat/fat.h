


#pragma once

#include <carbsup.h>

#define FAT_TAG ' taF'

#pragma pack(push, 1)

typedef struct _PART_ENTRY_MBR {
    UCHAR Attributes;
    UCHAR StartChs[ 3 ];
    UCHAR Type;
    UCHAR EndChs[ 3 ];
    ULONG StartLba;
    ULONG SectorCount;
} PART_ENTRY_MBR, *PPART_ENTRY_MBR;

typedef struct _PART_MBR {
    UCHAR          BootCode[ 440 ];
    ULONG          UniqueDiskId;
    USHORT         Reserved;
    PART_ENTRY_MBR Table[ 4 ];
    USHORT         BootSignature;
} PART_MBR, *PPART_MBR;

// potentially move to fat32 ?
typedef struct _DOS200BPB {
    CHAR        FileSystemIdentifier[ 8 ];
    USHORT      BytesPerSector;
    UCHAR       SectorsPerCluster;
    USHORT      ReservedSectors;
    UCHAR       FatCount;
    USHORT      RootDirectoryEntriesCount;
    USHORT      TotalSectors16;
    UCHAR       MediaDesciptor;
    USHORT      SectorsPerFat;
} DOS200BPB, *PDOS200BPB;

typedef struct _DOS331BPB {
    USHORT      SectorsPerTrack;
    USHORT      NumberOfHeads;
    ULONG       HiddenSectors;
    ULONG       TotalSectors32;
} DOS331BPB, *PDOS331BPB;

typedef struct _DOS701BPB {
    ULONG       SectorsPerFat;
    USHORT      MirroringFlags;
    USHORT      FatVersion;
    ULONG       RootDirectoryCluster;
    USHORT      FileSystemInfoSector;
    USHORT      BackupSectors;
    UCHAR       Reserved[ 12 ];
    UCHAR       BootDisk;
    UCHAR       NtFlags;
    UCHAR       ExtendedBootSignature;
    ULONG       VolumeSerialNumber;
    UCHAR       VolumeLabel[ 11 ];
    UCHAR       SystemIdentifierString[ 8 ];
} DOS701BPB, *PDOS701BPB;

typedef struct _BIOS_PARAMETER_BLOCK {
    CHAR        Jump[ 3 ];
    DOS200BPB   Dos2_00Bpb;
    DOS331BPB   Dos3_31Bpb;
    DOS701BPB   Dos7_01Bpb;     //EBPB
    UCHAR       BootCode[ 420 ];
    USHORT      BootSignature;
} BIOS_PARAMETER_BLOCK, *PBIOS_PARAMETER_BLOCK;

#define FAT32_READ_ONLY             0x01
#define FAT32_HIDDEN                0x02
#define FAT32_SYSTEM                0x04
#define FAT32_VOLUME_ID             0x08
#define FAT32_DIRECTORY             0x10
#define FAT32_ARCHIVE               0x20
#define FAT32_LFN                   (FAT32_READ_ONLY | FAT32_HIDDEN | FAT32_SYSTEM | FAT32_VOLUME_ID)

#define FAT32_END_OF_CHAIN          0x0FFFFFFF
#define FAT32_DIRECTORY_ENTRY_FREE  0xE5
#define FAT32_LAST_LFN_ENTRY        0x40

typedef union _FAT_DIRECTORY {
    struct {
        CHAR        Name[ 8 ];
        CHAR        Extension[ 3 ];
        UCHAR       Attributes;
        UCHAR       Reserved;
        UCHAR       CreateTimeTenth;
        USHORT      CreateTime;
        USHORT      CreateDate;
        USHORT      AccessDate;
        USHORT      ClusterHigh;
        USHORT      ModifiedTime;
        USHORT      ModifiedDate;
        USHORT      ClusterLow;
        ULONG       FileSize;
    } Short;

    struct {
        UCHAR       OrderOfEntry;
        WCHAR       First5Chars[ 5 ];
        UCHAR       Attributes;
        UCHAR       LongEntryType;
        UCHAR       NameChecksum;
        WCHAR       Next6Chars[ 6 ];
        USHORT      Zero;
        WCHAR       Next2Chars[ 2 ];
    } Long;
} FAT_DIRECTORY, *PFAT_DIRECTORY;

typedef struct _FAT32_FILE_SYSTEM_INFO {
    ULONG       Signature;
    UCHAR       Reserved[ 480 ];
    ULONG       Signature2;
    ULONG       LastKnownFreeClusterCount;
    ULONG       LastKnownFreeCluster;
    UCHAR       Reserved1[ 12 ];
    ULONG       Signature3;
} FAT32_FILE_SYSTEM_INFO, *PFAT32_FILE_SYSTEM_INFO;

#define FIRST_DATA_SECTOR( b )          ( ( b )->Dos2_00Bpb.ReservedSectors + ( ( b )->Dos2_00Bpb.FatCount * ( b )->Dos7_01Bpb.SectorsPerFat ) )
#define FIRST_SECTOR_OF_CLUSTER( b, n ) ( ( ( ( n )-2 ) * ( b )->Dos2_00Bpb.SectorsPerCluster ) + FIRST_DATA_SECTOR( b ) )

typedef enum _FAT_PATH_TYPE {
    PathInvalid,
    Path8Dot3,
    PathLongFileName,
    PathMaximum
} FAT_PATH_TYPE;

#pragma pack(pop)

typedef struct _FAT_DEVICE {
    BIOS_PARAMETER_BLOCK Bpb;
    PFAT_DIRECTORY       Root;
    NTSTATUS             RootStatus;
} FAT_DEVICE, *PFAT_DEVICE;

#define FspFatDevice( device ) ( ( PFAT_DEVICE )( device )->DeviceExtension )

typedef struct _FAT_FILE_CONTEXT {
    ULONG32  Flags;
    ULONG32* Chain;
    ULONG64  ChainLength;
    ULONG64  Length;

} FAT_FILE_CONTEXT, *PFAT_FILE_CONTEXT;

#define FspFatFileContext( file ) ( ( PFAT_FILE_CONTEXT )( file )->FsContext1 ) 

NTSTATUS
FspReadSectors(
    _In_ PDEVICE_OBJECT PartDevice,
    _In_ PVOID          Buffer,
    _In_ ULONG64        Length,
    _In_ ULONG64        Offset
);

NTSTATUS
FspQueryFatTable(
    _In_  PDEVICE_OBJECT DeviceObject,
    _In_  ULONG32        Index,
    _Out_ ULONG32*       Next
);

FAT_PATH_TYPE
FspValidateFileName(
    _In_ PWCHAR FileName
);

VOID
FspConvertPathTo8Dot3(
    _In_  PWCHAR FileName,
    _Out_ PCHAR  FileName8Dot3
);

UCHAR
FspNameChecksum(
    _In_ PCHAR ShortName
);

LONG
FspCompareLfnEntry(
    _In_ PFAT_DIRECTORY Directory,
    _In_ PWCHAR         Chars
);

ULONG64
FspFindDirectoryFile(
    _In_ PFAT_DIRECTORY Directory,
    _In_ FAT_PATH_TYPE  Type,
    _In_ PVOID          FileName
);

NTSTATUS
FsOpenFat32File(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP           Request
);

NTSTATUS
FspReadChain(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ ULONG32*       Chain,
    _In_ PVOID          Buffer,
    _In_ ULONG64        Length,
    _In_ ULONG64        Offset
);

NTSTATUS
FsQueryIndexFile(
    _In_  PDEVICE_OBJECT              DeviceObject,
    _In_  PFAT_DIRECTORY              Directory,
    _In_  ULONG64                     FileIndex,
    _In_  ULONG64                     Length,
    _Out_ PFILE_DIRECTORY_INFORMATION Information,
    _Out_ ULONG64*                    ReturnLength
);
