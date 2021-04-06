


#pragma once

#define IDE_TAG         ' EDI'

typedef struct _KIDE_CHANNEL *PKIDE_CHANNEL;
typedef struct _KIDE_CONTROL *PKIDE_CONTROL;
typedef struct _KIDE_DEVICE *PKIDE_DEVICE;

#pragma pack(push, 1)

//http://bswd.com/idems100.pdf
typedef struct _BM_PRDT_ENTRY {

    ULONG64 BaseAddress : 32;
    ULONG64 ByteCount : 16; // 0 - 0x10000
    ULONG64 Reserved : 15;
    ULONG64 EndOfTable : 1;

} BM_PRDT_ENTRY, *PBM_PRDT_ENTRY;

C_ASSERT( sizeof( BM_PRDT_ENTRY ) == 8 );


#pragma pack(pop)

typedef struct _KIDE_CHANNEL {
    USHORT Base;
    USHORT Control;
    USHORT BusMaster;
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

#define IDE_DEV_ATA     0x00
#define IDE_DEV_ATAPI   0x01

typedef struct _KIDE_DEVICE {
    KDISK_HEADER Header;


    BOOLEAN            Master;
    ULONG64            Type;
    ULONG64            Flags;
    ULONG64            SectorCount;

    NTSTATUS           BootStatus;
    PVOID              BootSector;

    DISK_GEOMETRY      Geometry;

    KATA_IDENTITY      Identity;
    PKIDE_CHANNEL      Channel;
    PKIDE_CONTROL      Control;
    KMUTEX             Lock;

    PMM_DMA_ADAPTER    DmaAdapter;
    PKEVENT            IrqEvent;
    PKMUTEX            IrqLock;

} KIDE_DEVICE, *PKIDE_DEVICE;

//4.2.1.
#define PIR_BM_SUPPORT              0x80
#define PIR_NATIVE_ENABLED          0x10
#define PIR_SECONDRAY_NATIVE_CAP    0x08
#define PIR_SECONDARY_MODE_NATIVE   0x04
#define PIR_PRIMARY_NATIVE_CAP      0x02
#define PIR_PRIMARY_MODE_NATIVE     0x01

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

#define BM_REG_COMMAND          0x00
#define BM_REG_STATUS           0x02
#define BM_REG_PRDT_ADDRESS     0x04

#define BM_CMD_START            0x01
#define BM_CMD_WRITE            0x08

#define BM_SR_SIMPLEX           0x80
#define BM_SR_ERR               0x08
#define BM_SR_TRC               0x04 // transfer complete
#define BM_SR_TRC_E             0x02 // transfer complete - exhaused
#define BM_SR_BSY               0x01

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
    _In_ PDEVICE_OBJECT PciDevice,
    _In_ PKIDE_CONTROL  Control,
    _In_ PKIDE_CHANNEL  Channel,
    _In_ BOOLEAN        Master,
    _In_ PKEVENT        IrqEvent,
    _In_ PKMUTEX        IrqLock
);

BOOLEAN
IdeDevice(
    _In_ PUNICODE_STRING LinkName,
    _In_ PDEVICE_OBJECT  DeviceObject,
    _In_ PDRIVER_OBJECT  DriverObject
);

BOOLEAN
IdeIrqService(
    _In_ PKINTERRUPT Interrupt,
    _In_ PKEVENT     IrqEvent
);
