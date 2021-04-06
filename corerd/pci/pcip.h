


#pragma once

#define PCI_MAX_BUSES               0x100
#define PCI_MAX_DEVICES             0x20
#define PCI_MAX_FUNCTIONS           0x8

#define PCI_TYPE_MULTIFUNC          0x80
#define PCI_TYPE_GENERIC            0x0
#define PCI_TYPE_PCI_BRIDGE         0x1
#define PCI_TYPE_PCI_CARDBUS_BRIDGE 0x2

#define PCI_CLASS_CODE_UNCLASSIFIED                             0x0
#define PCI_CLASS_CODE_MASS_STORAGE_CONTROLLER                  0x1
#define PCI_CLASS_CODE_NETWORK_CONTROLLER                       0x2
#define PCI_CLASS_CODE_DISPLAY_CONTROLLER                       0x3
#define PCI_CLASS_CODE_MULTIMEDIA_CONTROLLER                    0x4
#define PCI_CLASS_CODE_MEMORY_CONTROLLER                        0x5
#define PCI_CLASS_CODE_BRIDGE_DEVICE                            0x6
#define PCI_CLASS_CODE_SIMPLE_COMMUNICATION_CONTROLLER          0x7
#define PCI_CLASS_CODE_BASE_SYSTEM_PERIPHERAL                   0x8
#define PCI_CLASS_CODE_INPUT_DEVICE_CONTROLLER                  0x9
#define PCI_CLASS_CODE_DOCKING_STATION                          0xa
#define PCI_CLASS_CODE_PROCESSOR                                0xb
#define PCI_CLASS_CODE_SERIAL_BUS_CONTROLLER                    0xc
#define PCI_CLASS_CODE_WIRELESS_CONTROLLER                      0xd
#define PCI_CLASS_CODE_INTELLIGENT_CONTROLLER                   0xe
#define PCI_CLASS_CODE_SATELLITE_COMMUNICATION_CONTROLLER       0xf
#define PCI_CLASS_CODE_ENCRYPTION_CONTROLLER                    0x10
#define PCI_CLASS_CODE_SIGNAL_PROCESSING_CONTROLLER             0x11
#define PCI_CLASS_CODE_PROCESSING_ACCELERATOR                   0x12
#define PCI_CLASS_CODE_NON_ESSENTIAL_INSTRUMENTATION            0x13
#define PCI_CLASS_CODE_CO_PROCESSOR                             0x40
#define PCI_CLASS_CODE_UNASSIGNED_CLASS                         0xff

typedef union _PCI_BAR {
    struct {
        ULONG64 Mmio : 1;
        ULONG64 Low : 1;
        ULONG64 LongWidth : 1;
        ULONG64 Prefetch : 1;
        ULONG64 Address : 60;
    };

    struct {
        ULONG32 Lower;
        ULONG32 Upper;
    };

    ULONG64 Long;
} PCI_BAR, *PPCI_BAR;

C_ASSERT( sizeof( PCI_BAR ) == 8 );

#pragma pack(push, 1)

typedef struct _PCI_BRIDGE_DEVICE {
    PCI_DEVICE_HEADER Header;

    ULONG    Bar[ 2 ];

    UCHAR    PrimaryBusNumber;
    UCHAR    SecondaryBusNumber;
    UCHAR    SubordinateBusNumber;
    UCHAR    SecondaryLatencyTimer;

    UCHAR    IoBase;
    UCHAR    IoLimit;
    USHORT   SecondaryStatus;

    USHORT   MemoryBase;
    USHORT   MemoryLimit;

    USHORT   PrefetchMemoryBase;
    USHORT   PrefetchMemoryLimit;

    ULONG    PrefetchBaseUpper32;
    ULONG    PrefetchLimitUpper32;

    USHORT   IoLimitUpper;
    USHORT   IoBaseUpper;

    UCHAR    CapabilititesPointer;
    UCHAR    Reserved[ 3 ];

    ULONG    ExpansionROMBase;

    UCHAR    InterruptLine;
    UCHAR    InterruptPin;
    USHORT   BridgeControl;
} PCI_BRIDGE_DEVICE, *PPCI_BRIDGE_DEVICE;
typedef struct _PCI_CARDBUS_BRIDGE_DEVICE {
    PCI_DEVICE_HEADER Header;

    union {
        ULONG CardbusSocket;
        ULONG ExCaBaseAddress;
    };

    UCHAR     OffsetOfCapabilitiesList;
    UCHAR     Reserved;
    USHORT    SecondaryStatus;

    UCHAR     PciBusNumber;
    UCHAR     CardBusBusNumber;
    UCHAR     SubordinateBusNumber;
    UCHAR     CardBusLatencyTimer;

    struct {
        ULONG BaseAddress;
        ULONG Limit;
    } Memory[ 2 ];

    struct {
        ULONG BaseAddress;
        ULONG Limit;
    } Io[ 2 ];

    UCHAR     InterruptLine;
    UCHAR     InterruptPin;
    USHORT    BridgeControl;

    USHORT    SubsystemDeviceId;
    USHORT    SubsystemVendorId;

    ULONG     LegacyModeBaseAddress;

} PCI_CARDBUS_BRIDGE_DEVICE, *PPCI_CARDBUS_BRIDGE_DEVICE;

#pragma pack(pop)
