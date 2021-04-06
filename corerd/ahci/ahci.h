


#pragma once

#define AHCI_TAG            'ICHA'

typedef struct _KAHCI_DEVICE *PKAHCI_DEVICE;
typedef struct _KAHCI_CONTROL *PKAHCI_CONTROL;

#pragma pack(push, 1)

// ngl these ahci structures and names 
// are complete wank - i hate their "symbol" thing

//
// FUTURE NOTES: 
// 1. must check S64A bit in caps before assuming 64 bit logical addresses.
//

#define SATA_SIG_ATA            0x00000101  // SATA drive
#define SATA_SIG_ATAPI          0xEB140101  // SATAPI drive
#define SATA_SIG_SEMB           0xC33C0101  // Enclosure management bridge
#define SATA_SIG_PM             0x96690101  // Port multiplier

#define AHCI_DEV_NULL           0x00
#define AHCI_DEV_ATA            0x01
#define AHCI_DEV_ATAPI          0x02
#define AHCI_DEV_SEMB           0x03
#define AHCI_DEV_PM             0x04

#define HBA_PORT_IPM_ACTIVE     0x0100
#define HBA_PORT_DET_PRESENT    0x0003

#define HBA_CMD_ICC             0xFF000000
#define HBA_CMD_ICC_IDLE        0x00000000
#define HBA_CMD_ICC_ACTIVE      0x01000000
#define HBA_CMD_ICC_PARTIAL     0x02000000
#define HBA_CMD_ICC_SLUMBER     0x06000000
#define HBA_CMD_ICC_DEVSLEEP    0x08000000

/*
When system software writes a non-reserved value other than No-Op (0h), the HBA
shall perform the actions described above (for the value written) and update this field
back to Idle (0h).

If software writes to this field to change the state to a state the link is already in (i.e.
interface is in the active state and a request is made to go to the active state), the HBA
shall take no action and return this field to Idle. For all but DevSleep, if the interface is
in a low power state and software wants to transition to a different low power state,
software must first bring the link to active and then initiate the transition to the desired
low power state. If CAPS2.DESO is cleared to ‘0’, transition to DevSleep may occur
from any other interface state. If CAP2.DESO is set to ‘1’, then DevSleep may only be
transitioned to if the interface is in Slumber.
*/

typedef struct _KAHCI_HBA_PORT {
    ULONG64 CommandListLogical; // PxCLB
    ULONG64 FisLogical;         // PxFB
    ULONG32 InterruptStatus;    // PxIS
    ULONG32 InterruptEnable;    // PxIE
    ULONG32 CommandStatus;      // PxCMD
    ULONG32 Reserved0;
    ULONG32 TaskFileData;
    ULONG32 Signature;

    ULONG32 SataStatus;//SCR0
    ULONG32 SataControl;//SCR2
    ULONG32 SataError;//SCR1
    ULONG32 SataActive;//SCR3

    ULONG32 CommandIssue;
    ULONG32 SataNotification;//SCR4
    ULONG32 FisBasedSwitchControl;
    ULONG32 Reserved1[ 10 ];
    ULONG32 VendorSpecific[ 4 ];

} KAHCI_HBA_PORT, *PKAHCI_HBA_PORT;

typedef struct _KAHCI_HBA_MMIO {
    ULONG Caps1;
    ULONG GlobalControl;
    ULONG InterruptStatus;
    ULONG PortImplemented;
    ULONG Version;
    ULONG CccControl;
    ULONG CccPorts;
    ULONG EmLocation;
    ULONG EmControl;
    ULONG Caps2;
    ULONG ControlStatus;

    UCHAR Reserved[ 0xA0 - 0x2C ];

    UCHAR VendorSpecific[ 0x100 - 0xA0 ];

    KAHCI_HBA_PORT Ports[ 32 ];

} KAHCI_HBA_MMIO, *PKAHCI_HBA_MMIO;

typedef enum _FIS_TYPE0 {
    FIS_TYPE_REG_H2D = 0x27,    // Register FIS - host to device
    FIS_TYPE_REG_D2H = 0x34,    // Register FIS - device to host
    FIS_TYPE_DMA_ACT = 0x39,    // DMA activate FIS - device to host
    FIS_TYPE_DMA_SETUP = 0x41,  // DMA setup FIS - bidirectional
    FIS_TYPE_DATA = 0x46,   // Data FIS - bidirectional
    FIS_TYPE_BIST = 0x58,   // BIST activate FIS - bidirectional
    FIS_TYPE_PIO_SETUP = 0x5F,  // PIO setup FIS - device to host
    FIS_TYPE_DEV_BITS = 0xA1,   // Set device bits FIS - device to host
} FIS_TYPE0;

typedef UCHAR FIS_TYPE;

typedef struct _FIS_REG_H2D {
    FIS_TYPE Type;

    UCHAR PmPort : 4;
    UCHAR Reserved0 : 3;
    UCHAR Command : 1;//1 - command, 0 - control

    UCHAR CommandRegister;
    UCHAR FeatureLow;

    UCHAR BlockAddressLower[ 3 ];
    UCHAR DeviceRegister;

    UCHAR BlockAddressUpper[ 3 ];
    UCHAR FeatureHigh;

    USHORT Count;
    UCHAR IsochronousCommandCompletion;
    UCHAR ControlRegister;

    UCHAR Reserved1[ 4 ];
} FIS_REG_H2D, *PFIS_REG_H2D;

typedef struct _FIS_REG_D2H {
    FIS_TYPE Type;

    UCHAR PmPort : 4;
    UCHAR Reserved0 : 2;
    UCHAR Interrupt : 1;
    UCHAR Reserved1 : 1;

    UCHAR Status;
    UCHAR Error;

    UCHAR BlockAddressLower[ 3 ];
    UCHAR DeviceRegister;

    UCHAR BlockAddressUpper[ 3 ];
    UCHAR Reserved2;

    UCHAR CountLow;
    UCHAR CountHigh;
    UCHAR Reserved3[ 2 ];

    UCHAR Reserved4[ 4 ];
} FIS_REG_D2H, *PFIS_REG_D2H;

typedef struct _FIS_DATA {
    FIS_TYPE Type;

    UCHAR PmPort : 4;
    UCHAR Reserved0 : 4;

    UCHAR Reserved1[ 2 ];

    ULONG Data[ 1 ];
} FIS_DATA, *PFIS_DATA;

typedef struct _FIS_PIO_SETUP {
    FIS_TYPE Type;

    UCHAR PmPort : 4;
    UCHAR Reserved0 : 1;
    UCHAR Direction : 1;//1 - d2h
    UCHAR Interrupt : 1;
    UCHAR Reserved1 : 1;

    UCHAR Status;
    UCHAR Error;

    UCHAR LogicalBlockAddress0;
    UCHAR LogicalBlockAddress1;
    UCHAR LogicalBlockAddress2;
    UCHAR DeviceRegister;

    UCHAR LogicalBlockAddress3;
    UCHAR LogicalBlockAddress4;
    UCHAR LogicalBlockAddress5;
    UCHAR Reserved2;

    UCHAR CountLow;
    UCHAR CountHigh;
    UCHAR Reserved3;
    UCHAR NewStatus;

    USHORT TransferCount;
    UCHAR Reserved4[ 2 ];
} FIS_PIO_SETUP, *PFIS_PIO_SETUP;

typedef struct _FIS_DMA_SETUP {
    FIS_TYPE Type;

    UCHAR PmPort : 4;
    UCHAR Reserved0 : 1;
    UCHAR Direction : 1;//1 - d2h
    UCHAR Interrupt : 1;
    UCHAR Activate : 1; //specifies if dma activate fis is needed.

    UCHAR Reserved1[ 2 ];

    ULONG64 DmaBufferId;

    ULONG32 Reserved;

    ULONG32 DmaBufferOffset;//&~3, offs into buffer

    ULONG32 TransferCount;//&~1, bytes to transfer

    ULONG32 Reserved2;
} FIS_DMA_SETUP, *PFIS_DMA_SETUP;

typedef struct _KAHCI_HBA_FIS {

    FIS_DMA_SETUP DmaFis;
    UCHAR Pad0[ 4 ];

    FIS_PIO_SETUP PioFis;
    UCHAR Pad1[ 12 ];

    FIS_REG_D2H RegFis;
    UCHAR Pad2[ 4 ];

    ULONG64 DeviceBits;

    UCHAR UnknownFis[ 64 ];

    UCHAR Reserved[ 0x100 - 0xA0 ];

} KAHCI_HBA_FIS, *PKAHCI_HBA_FIS;

// spec only says 0xFF.. ?
C_ASSERT( sizeof( KAHCI_HBA_FIS ) == 0x100 );

typedef struct _KAHCI_HBA_COMMAND_HEADER {
    ULONG32 FisLength : 5; //in dwords.
    ULONG32 Atapi : 1;
    ULONG32 Write : 1; //1 - h2d, 0 - d2h
    ULONG32 Prefetchable : 1;
    ULONG32 Reset : 1;
    ULONG32 Bist : 1;
    ULONG32 ClearBusy : 1;
    ULONG32 Reserved0 : 1;
    ULONG32 PmPort : 4;
    ULONG32 PrdtLength : 16;

    ULONG32 PrdByteCount;
    ULONG64 CommandTableLogical;

    ULONG32 Reserved1[ 4 ];

} KAHCI_HBA_COMMAND_HEADER, *PKAHCI_HBA_COMMAND_HEADER;

C_ASSERT( sizeof( KAHCI_HBA_COMMAND_HEADER ) == 0x20 );

typedef struct _KAHCI_HBA_PRD {

    ULONG64 BaseAddress;
    ULONG32 Reserved0;

    //
    // Maximum of 0x400000, subtract 1
    //

    ULONG32 ByteCount : 22;
    ULONG32 Reserved1 : 9;
    ULONG32 Interrupt : 1;

} KAHCI_HBA_PRD, *PKAHCI_HBA_PRD;

C_ASSERT( sizeof( KAHCI_HBA_PRD ) == 0x10 );

typedef struct _KAHCI_HBA_COMMAND_TABLE {

    UCHAR CommandFis[ 64 ];
    UCHAR CommandAtapi[ 16 ];
    UCHAR Reserved[ 48 ];

    KAHCI_HBA_PRD Prdt[ 0 ]; // 0 ~ 65535

} KAHCI_HBA_COMMAND_TABLE, *PKAHCI_HBA_COMMAND_TABLE;

#define AHCI_CMD_TABLE_SIZE( PrdtLength ) \
( sizeof( KAHCI_HBA_COMMAND_TABLE ) + sizeof( KAHCI_HBA_PRD ) * PrdtLength )

#define AHCI_CMD_TABLE_ADDR( Base, PrdtLength, Index ) \
( PKAHCI_HBA_COMMAND_TABLE )( ( ( PUCHAR )( Base ) + AHCI_CMD_TABLE_SIZE( ( PrdtLength ) ) * ( Index ) ) )

#pragma pack(pop)

typedef struct _KAHCI_CONTROL {
    ULONG64         LogicalAddress;
    PKAHCI_HBA_MMIO Io;
    PIO_INTERRUPT   InterruptObject;
} KAHCI_CONTROL, *PKAHCI_CONTROL;

typedef struct _KAHCI_DEVICE {
    KDISK_HEADER              Header;

    ULONG64                   Type;
    ULONG64                   SectorCount;

    NTSTATUS                  BootStatus;
    PVOID                     BootSector;

    DISK_GEOMETRY             Geometry;

    KATA_IDENTITY             Identity;
    PKAHCI_CONTROL            Control;
    KSEMAPHORE                Semaphore;

    PMM_DMA_ADAPTER           DmaAdapter;

    PKAHCI_HBA_PORT           Port;
    PKAHCI_HBA_FIS            Fis;
    PKAHCI_HBA_COMMAND_HEADER CommandList;

    ULONG64                   CommandTableLogical;
    PKAHCI_HBA_COMMAND_TABLE  CommandTable;

} KAHCI_DEVICE, *PKAHCI_DEVICE;

//3.3.1
#define HBA_PxCMD_ST    ( 1 << 0 )  //start
#define HBA_PxCMD_FRE   ( 1 << 4 )  //fis recieve enable
#define HBA_PxCMD_FR    ( 1 << 14 ) //fis recieve
#define HBA_PxCMD_CR    ( 1 << 15 ) //command list running
#define HBA_PxIS_TFES   ( 1 << 30 ) //is task file error status


#define ATA_CMD_READ_DMA_EXT    0x25    /*7.29 READ DMA EXT - 25h, DMA */
#define ATA_CMD_WRITE_DMA_EXT   0x35
#define ATA_CMD_CACHE_FLUSH_EXT 0xEA
#define ATA_CMD_IDENTIFY        0xEC

NTSTATUS
AhciAccess(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ BOOLEAN        Write,
    _In_ ULONG64        BlockAddress,
    _In_ ULONG64        Length,
    _In_ PVOID          Buffer
);

BOOLEAN
AhciInitializeDevice(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT PciDevice,
    _In_ PKAHCI_CONTROL Control,
    _In_ PKAHCI_HBA_PORT      Port
);

VOID
AhciPortInitialize(
    _In_ PKAHCI_DEVICE Ahci
);

ULONG64
AhciFreeSlot(
    _In_ PKAHCI_DEVICE Ahci
);

BOOLEAN
AhciEngineSend(
    _In_ PKAHCI_DEVICE Ahci,
    _In_ ULONG64       Slot
);

VOID
AhciEngineStart(
    _In_ PKAHCI_DEVICE Ahci
);

VOID
AhciEngineStop(
    _In_ PKAHCI_DEVICE Ahci
);

NTSTATUS
AhciAccessInternal(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ BOOLEAN        Write,
    _In_ ULONG64        BlockAddress,
    _In_ ULONG64        Length,
    _In_ PVOID          Buffer
);
