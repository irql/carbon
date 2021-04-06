


#pragma once

#ifndef PCISYSAPI
#define PCISYSAPI DLLIMPORT
#endif

typedef BOOLEAN( *PKPCI_QUERY_DEVICE )(
    _In_ PUNICODE_STRING LinkName,
    _In_ PDEVICE_OBJECT  DeviceObject,
    _In_ PVOID           QueryContext
    );

#pragma pack(push, 1)
typedef struct _PCI_DEVICE_HEADER {
    USHORT   VendorId;
    USHORT   DeviceId;
    USHORT   Command;
    USHORT   Status;
    UCHAR    RevisionId;
    UCHAR    Prog_IF;
    UCHAR    SubClass;
    UCHAR    ClassCode;
    UCHAR    CacheLineSize;
    UCHAR    LatencyTimer;
    UCHAR    HeaderType;
    UCHAR    Bist;
} PCI_DEVICE_HEADER, *PPCI_DEVICE_HEADER;

typedef struct _PCI_STANDARD_DEVICE {
    PCI_DEVICE_HEADER Header;

    ULONG    Bar[ 6 ];

    ULONG    CardbusCisPointer;

    USHORT   SubsystemVendorId;
    USHORT   SubsystemId;
    ULONG    ExpansionROMBase;

    UCHAR    CapabilititesPointer;
    UCHAR    Reserved[ 7 ];

    UCHAR    InterruptLine;
    UCHAR    InterruptPin;
    UCHAR    MinGrant;
    UCHAR    MaxLatency;
} PCI_STANDARD_DEVICE, *PPCI_STANDARD_DEVICE;

typedef struct _PCI_DEVICE {
    ULONG32 Bus;
    ULONG32 Device;
    ULONG32 Function;

    //PCI_DEVICE_HEADER PciHeader;
    PCI_STANDARD_DEVICE PciDevice;
} PCI_DEVICE, *PPCI_DEVICE;

#define PCI_MSI_CAP_ID  0x05
typedef struct _KMSI_CAP {
    UCHAR   CapId;
    UCHAR   CapNext;
    USHORT  MessageControl;
    ULONG64 MessageAddress;
    USHORT  MessageData;
    USHORT  Reserved;
    ULONG32 Mask;
    ULONG32 Pending;
} KMSI_CAP, *PKMSI_CAP;
#pragma pack(pop)

typedef struct _PCI_BASE {
    ULONG64 Base;
    ULONG64 Size;
    ULONG32 Flags;
} PCI_BASE, *PPCI_BASE;

PCISYSAPI
VOID
PciQueryDevices(
    _In_ PWSTR              Format,
    _In_ PKPCI_QUERY_DEVICE QueryProcedure,
    _In_ PVOID              QueryContext
);

PCISYSAPI
VOID
PciReadBase(
    _In_  PPCI_DEVICE Device,
    _Out_ PPCI_BASE   Base,
    _In_  ULONG32     Index
);

PCISYSAPI
VOID
PciSetIoEnable(
    _In_ PPCI_DEVICE Device,
    _In_ BOOLEAN     Enable
);

PCISYSAPI
VOID
PciReadConfig(
    _In_  PPCI_DEVICE Device,
    _In_  ULONG32     Offset,
    _Out_ PVOID       Buffer,
    _In_  ULONG32     Length
);

PCISYSAPI
NTSTATUS
PciReadCap(
    _In_  PPCI_DEVICE Device,
    _In_  UCHAR       CapId,
    _Out_ PULONG32    Offset
);

//https://web.archive.org/web/20180107201056/http://www.xilinx.com/Attachment/PCI_SPEV_V3_0.pdf

#define PCI_CONFIG_ADDRESS          0xCF8
#define PCI_CONFIG_DATA             0xCFC

#define PCI_GET_ADDRESS( device, offset ) \
( ( ULONG32 )( device )->Bus << 16 ) | \
( ( ULONG32 )( device )->Device << 11 ) | \
( ( ULONG32 )( device )->Function << 8 ) | \
( ( ULONG32 )( offset ) & ~3 ) | ( 1 << 31 )

FORCEINLINE
UCHAR
PciRead8(
    _In_ PPCI_DEVICE Device,
    _In_ ULONG32     Offset
)
{
    __outdword( PCI_CONFIG_ADDRESS, PCI_GET_ADDRESS( Device, Offset ) );
    return __inbyte( PCI_CONFIG_DATA + ( Offset & 3 ) );
}

FORCEINLINE
USHORT
PciRead16(
    _In_ PPCI_DEVICE Device,
    _In_ ULONG32     Offset
)
{
    __outdword( PCI_CONFIG_ADDRESS, PCI_GET_ADDRESS( Device, Offset ) );
    return __inword( PCI_CONFIG_DATA + ( Offset & 2 ) );
}

FORCEINLINE
ULONG32
PciRead32(
    _In_ PPCI_DEVICE Device,
    _In_ ULONG32     Offset
)
{
    __outdword( PCI_CONFIG_ADDRESS, PCI_GET_ADDRESS( Device, Offset ) );
    return __indword( PCI_CONFIG_DATA );
}

FORCEINLINE
VOID
PciWrite8(
    _In_ PPCI_DEVICE Device,
    _In_ ULONG32     Offset,
    _In_ UCHAR       Value
)
{
    __outdword( PCI_CONFIG_ADDRESS, PCI_GET_ADDRESS( Device, Offset ) );
    __outbyte( PCI_CONFIG_DATA + ( Offset & 3 ), Value );
}

FORCEINLINE
VOID
PciWrite16(
    _In_ PPCI_DEVICE Device,
    _In_ ULONG32     Offset,
    _In_ USHORT      Value
)
{
    __outdword( PCI_CONFIG_ADDRESS, PCI_GET_ADDRESS( Device, Offset ) );
    __outword( PCI_CONFIG_DATA + ( Offset & 2 ), Value );
}

FORCEINLINE
VOID
PciWrite32(
    _In_ PPCI_DEVICE Device,
    _In_ ULONG32     Offset,
    _In_ ULONG32     Value
)
{
    __outdword( PCI_CONFIG_ADDRESS, PCI_GET_ADDRESS( Device, Offset ) );
    __outdword( PCI_CONFIG_DATA, Value );
}
