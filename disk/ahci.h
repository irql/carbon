

#pragma once


#define	SATA_SIG_ATA			0x00000101	// SATA drive
#define	SATA_SIG_ATAPI			0xEB140101	// SATAPI drive
#define	SATA_SIG_SEMB			0xC33C0101	// Enclosure management bridge
#define	SATA_SIG_PM				0x96690101	// Port multiplier

#define AHCI_DEV_NULL			0x00
#define AHCI_DEV_ATA			0x01
#define AHCI_DEV_SEMB			0x02
#define AHCI_DEV_PM				0x03
#define AHCI_DEV_ATAPI			0x04

#define HBA_PORT_IPM_ACTIVE		0x01
#define HBA_PORT_DET_PRESENT	0x03

typedef struct _HBA_PORT {
	//ULONG32 CommandListBaseLow;
	//ULONG32 CommandListBaseHigh;
	ULONG64 CommandListBase; // 1024 byte aligned

	//ULONG32 FisBaseLow;
	//ULONG32 FisBaseHigh;
	ULONG64 FisBase; //256 byte aligned
	ULONG32 InterruptStatus;
	ULONG32 InterruptEnable;
	ULONG32 CommandStatus;
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
	ULONG32 Reserved1[11];
	ULONG32 VendorSpecific[4];

} HBA_PORT, *PHBA_PORT;

typedef struct _HBA_MMIO {
	ULONG Capabilities;
	ULONG GlobalHostControl;
	ULONG InterruptStatus;
	ULONG PortImplemented;
	ULONG Version;
	ULONG CommandCompletionCoalescingControl;
	ULONG CommandCompletionCoalescingPorts;
	ULONG EnclosureManagementLocation;
	ULONG EnclosureManagementControl;
	ULONG CapabilitiesExt;
	ULONG ControlStatus;

	UCHAR Reserved[0xA0 - 0x2C];

	UCHAR VendorSpecific[0x100 - 0xA0];

	HBA_PORT Ports[32];

} HBA_MMIO, *PHBA_MMIO;

typedef enum _FIS_TYPE0 {
	FIS_TYPE_REG_H2D = 0x27,	// Register FIS - host to device
	FIS_TYPE_REG_D2H = 0x34,	// Register FIS - device to host
	FIS_TYPE_DMA_ACT = 0x39,	// DMA activate FIS - device to host
	FIS_TYPE_DMA_SETUP = 0x41,	// DMA setup FIS - bidirectional
	FIS_TYPE_DATA = 0x46,	// Data FIS - bidirectional
	FIS_TYPE_BIST = 0x58,	// BIST activate FIS - bidirectional
	FIS_TYPE_PIO_SETUP = 0x5F,	// PIO setup FIS - device to host
	FIS_TYPE_DEV_BITS = 0xA1,	// Set device bits FIS - device to host
} FIS_TYPE0;

typedef UCHAR FIS_TYPE;

typedef struct _FIS_REG_H2D {
	FIS_TYPE Type;

	UCHAR PmPort : 4;
	UCHAR Reserved0 : 3;
	UCHAR Command : 1;//1 - command, 0 - control

	UCHAR CommandRegister;
	UCHAR FeatureLow;

	UCHAR LogicalBlockAddress0;
	UCHAR LogicalBlockAddress1;
	UCHAR LogicalBlockAddress2;
	UCHAR DeviceRegister;

	UCHAR LogicalBlockAddress3;
	UCHAR LogicalBlockAddress4;
	UCHAR LogicalBlockAddress5;
	UCHAR FeatureHigh;

	USHORT Count;
	UCHAR IsochronousCommandCompletion;
	UCHAR ControlRegister;

	UCHAR Reserved1[4];
} FIS_REG_H2D, *PFIS_REG_H2D;

typedef struct _FIS_REG_D2H {
	FIS_TYPE Type;

	UCHAR PmPort : 4;
	UCHAR Reserved0 : 2;
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
	UCHAR Reserved3[2];

	UCHAR Reserved4[4];
} FIS_REG_D2H, *PFIS_REG_D2H;

typedef struct _FIS_DATA {
	FIS_TYPE Type;

	UCHAR PmPort : 4;
	UCHAR Reserved0 : 4;

	UCHAR Reserved1[2];

	ULONG Data[1];
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
	UCHAR Reserved4[2];
} FIS_PIO_SETUP, *PFIS_PIO_SETUP;

typedef struct _FIS_DMA_SETUP {
	FIS_TYPE Type;

	UCHAR PmPort : 4;
	UCHAR Reserved0 : 1;
	UCHAR Direction : 1;//1 - d2h
	UCHAR Interrupt : 1;
	UCHAR Activate : 1; //specifies if dma activate fis is needed.

	UCHAR Reserved1[2];

	ULONG64 DmaBufferId;

	ULONG32 Reserved;

	ULONG32 DmaBufferOffset;//&~3, offs into buffer

	ULONG32 TransferCount;//&~1, bytes to transfer

	ULONG32 Reserved2;
} FIS_DMA_SETUP, *PFIS_DMA_SETUP;

typedef struct _HBA_FIS {

	FIS_DMA_SETUP DmaFis;
	UCHAR Pad0[4];

	FIS_PIO_SETUP PioFis;
	UCHAR Pad1[12];

	FIS_REG_D2H RegFis;
	UCHAR Pad2[4];

	//FIS_DEV_BITS DevFis;
	ULONG64 DevBitsFis;

	UCHAR ufis[64];

	UCHAR Reserved[0x100 - 0xA0];

} HBA_FIS, *PHBA_FIS;

typedef struct _HBA_COMMAND_HEADER {
	UCHAR FisLength : 5; //in dwords.
	UCHAR Atapi : 1;
	UCHAR Write : 1; //1 - h2d, 0 - d2h
	UCHAR Prefetchable : 1; 

	UCHAR Reset : 1;
	UCHAR Bist : 1;
	UCHAR ClearBusy : 1;
	UCHAR Reserved0 : 1;
	UCHAR PmPort : 4;

	USHORT PrdtLength;

	ULONG PrdBytesTransferred;
	
	ULONG64 CommandTableDescriptorBase;

	ULONG Reserved1[4];

} HBA_COMMAND_HEADER, *PHBA_COMMAND_HEADER;

typedef struct _HBA_PRDT_ENTRY {

	ULONG64 DataBaseAddress;
	ULONG32 Reserved0;

	ULONG32 ByteCount : 22;//0x400000 max (4m)
	ULONG32 Reserved1 : 9;
	ULONG32 Interrupt : 1; //on completion.

} HBA_PRDT_ENTRY, *PHBA_PRDT_ENTRY;

typedef struct _HBA_COMMAND_TABLE {

	UCHAR CommandFis[64];

	UCHAR CommandAtapi[16];//12|16

	UCHAR Reserved[48];

//#pragma warning(disable:4200)
	HBA_PRDT_ENTRY PrdtEntries[1]; // 0 ~ 65535
//#pragma warning(default:4200)

} HBA_COMMAND_TABLE, *PHBA_COMMAND_TABLE;

typedef struct _AHCI_CONTROLLER {
	PPCI_DEVICE PciDevice;

	ULONG64 Physical;
	PHBA_MMIO Io;

	

} AHCI_CONTROLLER, *PAHCI_CONTROLLER;

typedef struct _AHCI_DISK_DATA {
	/*
		DiskObject->ControllerData
	*/

	PAHCI_CONTROLLER Ahci;
	PHBA_PORT Port;
	PHBA_FIS Fis;
	PHBA_COMMAND_HEADER CommandList;
	ULONG64 CommandTablePhysical;
	PHBA_COMMAND_TABLE CommandTable;
	
	ATA_DEVICE_IDENTITY Identity;

	ULONG64 Size;
	PDISK_GEOMETRY Geometry;
	ULONG32 Type;

} AHCI_DISK_DATA, *PAHCI_DISK_DATA;

//3.3.1
#define HBA_PxCMD_ST    0x0001//start
#define HBA_PxCMD_FRE   0x0010//fis recieve enable
#define HBA_PxCMD_FR    0x4000//fis recieve
#define HBA_PxCMD_CR    0x8000//command list running
#define HBA_PxIS_TFES	(1 << 30) //is task file error status