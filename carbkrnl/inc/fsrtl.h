


#pragma once

NTSYSAPI
VOID
FsRtlCreateDiskDevice(
    _In_ PUNICODE_STRING DriveDirectory
);

NTSYSAPI
ULONG64
FsRtlQueryDiskCount(

);

NTSYSAPI
BOOLEAN
FsRtlContainingDirectory(
    _Inout_ PUNICODE_STRING Complete,
    _Inout_ PUNICODE_STRING Directory
);

NTSYSAPI
BOOLEAN
FsRtlFileName(
    _Inout_ PUNICODE_STRING Complete,
    _Inout_ PUNICODE_STRING Directory
);

NTSYSAPI
CHAR
FsRtlNextDriveLetter(

);

NTSYSAPI
USHORT
FsRtlFileNameIndex(
    _In_ PUNICODE_STRING Directory
);

typedef struct _DISK_GEOMETRY {
    ULONG64 SectorSize;
    ULONG64 Cylinders;
    ULONG64 Heads;
    ULONG64 SectorsPerTrack;
} DISK_GEOMETRY, *PDISK_GEOMETRY;

#pragma pack(push, 1)

typedef union _KATA_IDENTITY {
    struct {
        USHORT Buffer[ 256 ];
    };

    struct {

        USHORT DeviceType;
        USHORT Cylinders;
        USHORT SpecificConfirguration;
        USHORT Heads;
        UCHAR Reserved[ 4 ];
        USHORT SectorsPerTrack;
        UCHAR Reserved1[ 6 ];
        UCHAR SerialNumber[ 20 ];
        UCHAR Reserved2[ 6 ];
        UCHAR FirmwareRevision[ 8 ];
        //UCHAR ModelNumber[ 40 ];
        USHORT ModelNumber[ 20 ];
        UCHAR Reserved3[ 4 ];

        ULONG Capabilities;
        UCHAR Reserved4[ 6 ];
        USHORT CurrentLogicalCylinders;
        USHORT CurrentLogicalHeads;
        USHORT CurrentLogicalSectorsPerTrack;
        ULONG CurrentCapacityInSectors;
        UCHAR Reserved5[ 2 ];
        ULONG MaxLogicalBlockAddress;//0x0FFFFFFF
        UCHAR Reserved6[ 36 ];
        USHORT MajorVersionNumber;
        USHORT MinorVersionNumber;

        ULONG CommandSets1;
        ULONG CommandSets2;
        ULONG CommandSets3;

        UCHAR Reserved7[ 24 ];

        ULONG64 MaxLogicalBlockAddressExt; //&0x0000FFFFFFFFFFFF 
    };
} KATA_IDENTITY, *PKATA_IDENTITY;

#pragma pack(pop)

#define ATA_SR_BSY              0x80// Busy
#define ATA_SR_DRDY             0x40// Drive ready
#define ATA_SR_DF               0x20// Drive write fault
#define ATA_SR_DSC              0x10// Drive seek complete
#define ATA_SR_DRQ              0x08// Data request ready
#define ATA_SR_CORR             0x04// Corrected Value
#define ATA_SR_IDX              0x02// Index
#define ATA_SR_ERR              0x01// Status

#define ATA_ER_BBK              0x80// Bad block
#define ATA_ER_UNC              0x40// Uncorrectable Value
#define ATA_ER_MC               0x20// Media changed
#define ATA_ER_IDNF             0x10// ID mark not found
#define ATA_ER_MCR              0x08// Media change request
#define ATA_ER_ABRT             0x04// Command aborted
#define ATA_ER_TK0NF            0x02// Track 0 not found
#define ATA_ER_AMNF             0x01// No address mark
