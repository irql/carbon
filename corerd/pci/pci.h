


#pragma once

#ifndef PCISYSAPI
#define PCISYSAPI DLLIMPORT
#endif

typedef BOOLEAN( *PKPCI_QUERY_DEVICE )(
    _In_ PUNICODE_STRING LinkName,
    _In_ PDEVICE_OBJECT  DeviceObject,
    _In_ PVOID           QueryContext
    );

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

    PCI_DEVICE_HEADER PciHeader;
} PCI_DEVICE, *PPCI_DEVICE;

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
