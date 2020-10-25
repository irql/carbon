

#pragma once

typedef struct _IDE_CHANNEL_REGISTERS {
	USHORT Base;
	USHORT Control;
	USHORT BusMasterIde;
	UCHAR  nIEN;
} IDE_CHANNEL_REGISTERS, *PIDE_CHANNEL_REGISTERS;

typedef struct _IDE_CONTROLLER {
	PPCI_DEVICE PciDevice;

	BOOLEAN IdeIrqInvoked;

	struct {
		UCHAR Exists : 1;
		UCHAR Channel : 1;
		UCHAR Drive : 1;
		UCHAR Type : 1;

		USHORT MajorVersionNumber;
		USHORT MinorVersionNumber;

		ULONG64 Size;

		ATA_DEVICE_IDENTITY Identity;
	} Devices[ 4 ];

	IDE_CHANNEL_REGISTERS Channels[ 2 ];

} IDE_CONTROLLER, *PIDE_CONTROLLER;

typedef struct _IDE_DISK_DATA {
	/*
		DiskObject->ControllerData
	*/

	PIDE_CONTROLLER Ide;
	UCHAR Drive;

	PDISK_GEOMETRY Geometry;

} IDE_DISK_DATA, *PIDE_DISK_DATA;

#define ATA_PRIMARY		0x00
#define ATA_SECONDARY	0x01

#define IDE_ATA			0x00
#define IDE_ATAPI		0x01

#define ATA_MASTER		0x00
#define ATA_SLAVE		0x01


