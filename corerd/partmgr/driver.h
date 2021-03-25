


#pragma once

#include <carbsup.h>

#define PART_TAG    'traP'

#define MBR_ATTRIBUTE_ACTIVE 0x80

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

#pragma pack(pop)

typedef struct _PART_DEVICE {
    ULONG64 StartLba;
    ULONG64 SectorCount;
    ULONG64 SerialNumber;
    //PVOID   BootSector;
    UCHAR   BootSector[ 512 ];

} PART_DEVICE, *PPART_DEVICE;

NTSTATUS
FsCreateMbrPartTable(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING Disk
);