


#pragma once

#define IDE_TAG         ' EDI'

typedef struct _KIDE_CHANNEL *PKIDE_CHANNEL;
typedef struct _KIDE_CONTROL *PKIDE_CONTROL;
typedef union _KIDE_IDENTITY *PKIDE_IDENTITY;
typedef struct _KIDE_DEVICE *PKIDE_DEVICE;

#pragma pack(push, 1)

//move.
typedef struct _DISK_GEOMETRY {
    ULONG64 SectorSize;
    ULONG64 Cylinders;
    ULONG64 Heads;
    ULONG64 SectorsPerTrack;
} DISK_GEOMETRY, *PDISK_GEOMETRY;

typedef union _KIDE_IDENTITY {
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
} KIDE_IDENTITY, *PKIDE_IDENTITY;
#pragma pack(pop)

typedef struct _KIDE_CHANNEL {
    USHORT Base;
    USHORT Control;
    USHORT BusMaster;
    UCHAR  NoInterrupt;
} KIDE_CHANNEL, *PKIDE_CHANNEL;

typedef struct _KIDE_CONTROL {
    KIDE_CHANNEL  Primary;
    KIDE_CHANNEL  Secondary;

    KEVENT        IrqEvent;
    BOOLEAN       IrqPending;

    KSPIN_LOCK    DeviceLock;

} KIDE_CONTROL, *PKIDE_CONTROL;

#define IDE_CHS     0x01
#define IDE_LBA28   0x02
#define IDE_LBA48   0x04
#define IDE_DMA     0x08

typedef struct _KIDE_DEVICE {
    DISK_OBJECT_HEADER Header;

    UCHAR          Partition : 1;
    UCHAR          Packet : 1;
    UCHAR          Drive : 1;
    UCHAR          Flags : 5;

    ULONG64        SectorCount;
    PDEVICE_OBJECT DriveDevice;

    NTSTATUS       BootStatus;
    PVOID          BootSector;

    DISK_GEOMETRY  Geometry;

    KIDE_IDENTITY  Identity;
    PKIDE_CHANNEL  Channel;
    PKIDE_CONTROL  Control;
    KSPIN_LOCK     IdeLock;

} KIDE_DEVICE, *PKIDE_DEVICE;

#define ATA_REG_DATA            0x00
#define ATA_REG_ERROR           0x01
#define ATA_REG_FEATURES        0x01
#define ATA_REG_SECCOUNT0       0x02
#define ATA_REG_LBA0            0x03
#define ATA_REG_LBA1            0x04
#define ATA_REG_LBA2            0x05
#define ATA_REG_HDDEVSEL        0x06
#define ATA_REG_COMMAND         0x07
#define ATA_REG_STATUS          0x07
#define ATA_REG_CONTROL         0x08 // not a reg

#define BM_REG_CONTROL          0x00
#define BM_REG_STATUS           0x02
#define BM_REG_PRDT_ADDRESS     0x04

#define ATA_CMD_READ_PIO        0x20
#define ATA_CMD_READ_PIO_EXT    0x24
#define ATA_CMD_READ_DMA        0xC8    /*7.28 READ DMA - C8h, DMA */
#define ATA_CMD_READ_DMA_EXT    0x25    /*7.29 READ DMA EXT - 25h, DMA */
#define ATA_CMD_WRITE_PIO       0x30
#define ATA_CMD_WRITE_PIO_EXT   0x34
#define ATA_CMD_WRITE_DMA       0xCA
#define ATA_CMD_WRITE_DMA_EXT   0x35
#define ATA_CMD_CACHE_FLUSH     0xE7
#define ATA_CMD_CACHE_FLUSH_EXT 0xEA
#define ATA_CMD_PACKET          0xA0
#define ATA_CMD_IDENTIFY_PACKET 0xA1
#define ATA_CMD_IDENTIFY        0xEC

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

FORCEINLINE
VOID
BmWrite(
    _In_ PKIDE_CHANNEL Channel,
    _In_ USHORT        Register,
    _In_ ULONG32       Value
)
{
    if ( Register == BM_REG_PRDT_ADDRESS ) {
        __outdword( Channel->BusMaster + Register, Value );
    }
    else {
        __outbyte( Channel->BusMaster + Register, ( UCHAR )Value );
    }
}

FORCEINLINE
ULONG32
BmRead(
    _In_ PKIDE_CHANNEL Channel,
    _In_ USHORT        Register
)
{
    if ( Register == BM_REG_PRDT_ADDRESS ) {
        return __indword( Channel->BusMaster + Register );
    }
    else {
        return __inbyte( Channel->BusMaster + Register );
    }
}

FORCEINLINE
VOID
IdeWrite(
    _In_ PKIDE_CHANNEL Channel,
    _In_ USHORT        Register,
    _In_ UCHAR         Value
)
{
    if ( Register == ATA_REG_CONTROL ) {
        __outbyte( Channel->Control, Value );
    }
    else {
        __outbyte( Channel->Base + Register, Value );
    }
}

FORCEINLINE
UCHAR
IdeRead(
    _In_ PKIDE_CHANNEL Channel,
    _In_ USHORT        Register
)
{
    if ( Register == ATA_REG_CONTROL ) {
        return __inbyte( Channel->Control );
    }
    else {
        return __inbyte( Channel->Base + Register );
    }
}

NTSTATUS
IdeAccess(
    _In_ PDEVICE_OBJECT   DeviceObject,
    _In_ BOOLEAN          Write,
    _In_ ULONG64          BlockAddress,
    _In_ ULONG64          Length,
    _In_ PVOID            Buffer
);

NTSTATUS
IdeAccessInternal(
    _In_ PDEVICE_OBJECT   DeviceObject,
    _In_ BOOLEAN          Write,
    _In_ ULONG64          BlockAddress,
    _In_ ULONG64          Length,
    _In_ PVOID            Buffer
);

BOOLEAN
IdeInitializeDevice(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PKIDE_CONTROL  Control,
    _In_ PKIDE_CHANNEL  Channel,
    _In_ UCHAR          Drive
);

BOOLEAN
IdeDevice(
    _In_ PUNICODE_STRING LinkName,
    _In_ PDEVICE_OBJECT  DeviceObject,
    _In_ PDRIVER_OBJECT  DriverObject
);
