/*++

Module ObjectName:

	hal.h

Abstract:

	.

--*/

#pragma once

#define E820_USABLE				0x1
#define E820_UNUSABLE			0x2
#define E820_ACPI_RECLAIMABLE	0x3
#define E820_ACPI_NVS_MEMORY	0x4
#define E820_BAD_MEMORY			0x5
//anything not 1,2,3,4 is bad.

typedef struct _E820MM_ENTRY {
	ULONG64			RegionBase;
	ULONG64			RegionLength;
	ULONG32			RegionType;
	ULONG32			AcpiExtendedAttributes;
} E820MM_ENTRY, *PE820MM_ENTRY;

typedef struct _E820MM {
	ULONG32			EntryCount;
	E820MM_ENTRY	Entries[ 1 ];
} E820MM, *PE820MM;

#define PCI_CONFIG_ADDRESS			0xcf8
#define PCI_CONFIG_DATA				0xcfc

#define PCI_ENABLE_BIT				(1<<31)

#define PCI_MAX_BUSES				0x100
#define PCI_MAX_DEVICES				0x20
#define PCI_MAX_FUNCTIONS			0x8

#define PCI_BAR_IO					0x1
#define PCI_BAR_LOW_MEMORY			0x2
#define PCI_BAR_64					0x4
#define PCI_BAR_PREFETCH			0x8

#define PCI_COMMAND_BUS_MASTER		(1<<2)

#define PCI_TYPE_MULTIFUNC			0x80
#define PCI_TYPE_GENERIC			0x0
#define PCI_TYPE_PCI_BRIDGE			0x1
#define PCI_TYPE_PCI_CARDBUS_BRIDGE 0x2

#define PCI_CLASS_CODE_UNCLASSIFIED							0x0
#define PCI_CLASS_CODE_MASS_STORAGE_CONTROLLER				0x1
#define PCI_CLASS_CODE_NETWORK_CONTROLLER					0x2
#define PCI_CLASS_CODE_DISPLAY_CONTROLLER					0x3
#define PCI_CLASS_CODE_MULTIMEDIA_CONTROLLER				0x4
#define PCI_CLASS_CODE_MEMORY_CONTROLLER					0x5
#define PCI_CLASS_CODE_BRIDGE_DEVICE						0x6
#define PCI_CLASS_CODE_SIMPLE_COMMUNICATION_CONTROLLER		0x7
#define PCI_CLASS_CODE_BASE_SYSTEM_PERIPHERAL				0x8
#define PCI_CLASS_CODE_INPUT_DEVICE_CONTROLLER				0x9
#define PCI_CLASS_CODE_DOCKING_STATION						0xa
#define PCI_CLASS_CODE_PROCESSOR							0xb
#define PCI_CLASS_CODE_SERIAL_BUS_CONTROLLER				0xc
#define PCI_CLASS_CODE_WIRELESS_CONTROLLER					0xd
#define PCI_CLASS_CODE_INTELLIGENT_CONTROLLER				0xe
#define PCI_CLASS_CODE_SATELLITE_COMMUNICATION_CONTROLLER	0xf
#define PCI_CLASS_CODE_ENCRYPTION_CONTROLLER				0x10
#define PCI_CLASS_CODE_SIGNAL_PROCESSING_CONTROLLER			0x11
#define PCI_CLASS_CODE_PROCESSING_ACCELERATOR				0x12
#define PCI_CLASS_CODE_NON_ESSENTIAL_INSTRUMENTATION		0x13
#define PCI_CLASS_CODE_CO_PROCESSOR							0x40
#define PCI_CLASS_CODE_UNASSIGNED_CLASS						0xff

#define PCI_COMMAND_BUS_MASTER		(1<<2)

typedef struct _PCI_DEVICE_HEADER {
	USHORT	VendorId;
	USHORT	DeviceId;
	USHORT	Command;
	USHORT	Status;
	UCHAR	RevisionId;
	UCHAR	Prog_IF;
	UCHAR	SubClass;
	UCHAR	ClassCode;
	UCHAR	CacheLineSize;
	UCHAR	LatencyTimer;
	UCHAR	HeaderType;
	UCHAR	Bist;
} PCI_DEVICE_HEADER, *PPCI_DEVICE_HEADER;

typedef struct _PCI_STANDARD_DEVICE {
	PCI_DEVICE_HEADER Header;

	ULONG	Bar[ 6 ];

	ULONG	CardbusCisPointer;

	USHORT	SubsystemVendorId;
	USHORT	SubsystemId;
	ULONG	ExpansionROMBase;

	UCHAR	CapabilititesPointer;
	UCHAR	Reserved[ 7 ];

	UCHAR	InterruptLine;
	UCHAR	InterruptPin;
	UCHAR	MinGrant;
	UCHAR	MaxLatency;
} PCI_STANDARD_DEVICE, *PPCI_STANDARD_DEVICE;

typedef struct _PCI_BRIDGE_DEVICE {
	PCI_DEVICE_HEADER Header;

	ULONG	Bar[ 2 ];

	UCHAR	PrimaryBusNumber;
	UCHAR	SecondaryBusNumber;
	UCHAR	SubordinateBusNumber;
	UCHAR	SecondaryLatencyTimer;

	UCHAR	IoBase;
	UCHAR	IoLimit;
	USHORT	SecondaryStatus;

	USHORT	MemoryBase;
	USHORT	MemoryLimit;

	USHORT	PrefetchMemoryBase;
	USHORT	PrefetchMemoryLimit;

	ULONG	PrefetchBaseUpper32;
	ULONG	PrefetchLimitUpper32;

	USHORT	IoLimitUpper;
	USHORT	IoBaseUpper;

	UCHAR	CapabilititesPointer;
	UCHAR	Reserved[ 3 ];

	ULONG	ExpansionROMBase;

	UCHAR	InterruptLine;
	UCHAR	InterruptPin;
	USHORT	BridgeControl;
} PCI_BRIDGE_DEVICE, *PPCI_BRIDGE_DEVICE;

typedef struct _PCI_CARDBUS_BRIDGE_DEVICE {
	PCI_DEVICE_HEADER Header;

	union {
		ULONG CardbusSocket;
		ULONG ExCaBaseAddress;
	};

	UCHAR	OffsetOfCapabilitiesList;
	UCHAR	Reserved;
	USHORT	SecondaryStatus;

	UCHAR	PciBusNumber;
	UCHAR	CardBusBusNumber;
	UCHAR	SubordinateBusNumber;
	UCHAR	CardBusLatencyTimer;
	/*
	ULONG	MemoryBaseAddress0;
	ULONG	MemoryLimit0;
	ULONG	MemoryBaseAddress1;
	ULONG	MemoryLimit1;

	ULONG	IoBaseAddress0;
	ULONG	IoLimit0;
	ULONG	IoBaseAddress1;
	ULONG	IoLimit1;
	*/

	struct {
		ULONG BaseAddress;
		ULONG Limit;
	} Memory[ 2 ];

	struct {
		ULONG BaseAddress;
		ULONG Limit;
	} Io[ 2 ];

	UCHAR	InterruptLine;
	UCHAR	InterruptPin;
	USHORT	BridgeControl;

	USHORT	SubsystemDeviceId;
	USHORT	SubsystemVendorId;

	ULONG	LegacyModeBaseAddress;

} PCI_CARDBUS_BRIDGE_DEVICE, *PPCI_CARDBUS_BRIDGE_DEVICE;

typedef struct _PCI_DEVICE {
	USHORT	Bus;
	UCHAR	Device;
	UCHAR	Function;

	PCI_DEVICE_HEADER PciHeader;
} PCI_DEVICE, *PPCI_DEVICE;

typedef struct _PCI_BASE_ADDRESS_REGISTER {
	ULONG64 Base;
	ULONG64 Size;
	ULONG32 Flags;
} PCI_BASE_ADDRESS_REGISTER, *PPCI_BASE_ADDRESS_REGISTER;

typedef struct _PCI_DEVICE_LIST {
	ULONG32			DeviceCount;
	PPCI_DEVICE		PciDevices;
} PCI_DEVICE_LIST, *PPCI_DEVICE_LIST;

NTSYSAPI EXTERN PCI_DEVICE_LIST HalPciDeviceList;

NTSYSAPI
VOID
HalPciReadBar(
	__in PPCI_DEVICE Device,
	__out PPCI_BASE_ADDRESS_REGISTER BaseAddressRegister,
	__in UCHAR Index
);

NTSYSAPI
VOID
HalPciSetIoEnable(
	__in PPCI_DEVICE Device,
	__in BOOLEAN Enable
);

typedef struct _KTRAP_FRAME {
	UCHAR x87save[ 4096 ];
	ULONG64 Cr3;
	ULONG64 DataSegment;
	ULONG64 R15, R14, R13, R12, R11, R10, R9, R8, Rdi, Rsi, Rbp, Rbx, Rdx, Rcx, Rax;
	ULONG64 Interrupt, Error;
	ULONG64 Rip, CodeSegment, Rflags, Rsp, StackSegment;
} KTRAP_FRAME, *PKTRAP_FRAME;

typedef struct _VBE_INFO {
	USHORT Width;
	USHORT Height;
	UCHAR Bpp;
	ULONG32 Framebuffer;
} VBE_INFO, *PVBE_INFO;

NTSYSAPI
UCHAR
HalPciRead8(
	__in PPCI_DEVICE Device,
	__in ULONG Offset
);

NTSYSAPI
USHORT
HalPciRead16(
	__in PPCI_DEVICE Device,
	__in ULONG Offset
);

NTSYSAPI
ULONG32
HalPciRead32(
	__in PPCI_DEVICE Device,
	__in ULONG Offset
);

NTSYSAPI
VOID
HalPciWrite8(
	__in PPCI_DEVICE Device,
	__in ULONG Offset,
	__in UCHAR Value
);

NTSYSAPI
VOID
HalPciWrite16(
	__in PPCI_DEVICE Device,
	__in ULONG Offset,
	__in USHORT Value
);

NTSYSAPI
VOID
HalPciWrite32(
	__in PPCI_DEVICE Device,
	__in ULONG Offset,
	__in ULONG Value
);

typedef struct _GDO_INFO {
	ULONG32* Framebuffer;
	USHORT Width;
	USHORT Height;
} GDI_INFO, *PGDI_INFO;

NTSYSAPI
PGDI_INFO
VbeGetInfo(

);

NTSYSAPI
ULONG32
HalIoApicRead(
	__in ULONG64 IoApicAddress,
	__in ULONG32 Register
);

NTSYSAPI
VOID
HalIoApicWrite(
	__in ULONG64 IoApicAddress,
	__in ULONG32 Register,
	__in ULONG32 Value
);


#define IO_APIC_ID						0x00
#define IO_APIC_VER						0x01
#define IO_APIC_ARB						0x02
#define IO_APIC_REDIRECTION_TABLE(n)	(0x10 + 2 * n)

typedef enum _DELIVERY_MODE {
	DeliveryModeEdge,
	DeliveryModeLevel
} DELIVERY_MODE;

typedef enum _DESTINATION_MODE {
	DestinationModePhysical,
	DestinationModeLogical
} DESTINATION_MODE;

typedef union _REDIRECTION_ENTRY {

	struct {
		ULONG64 InterruptVector : 8;
		ULONG64 DeliveryMode : 3;
		ULONG64 DestinationMode : 1;
		ULONG64 DeliveryStatus : 1;
		ULONG64 PinPolarity : 1;
		ULONG64 RemoteIrr : 1;
		ULONG64 TriggerMode : 1;
		ULONG64 Mask : 1;
		ULONG64 Reserved : 39;
		ULONG64 Destination : 8;
	};

	struct {
		ULONG32 Lower;
		ULONG32 Upper;
	};

} REDIRECTION_ENTRY, *PREDIRECTION_ENTRY;

NTSYSAPI
VOID
HalIoApicRedirectIrq(
	__in UCHAR Irq,
	__in PREDIRECTION_ENTRY RedirectionEntry
);

NTSYSAPI
VOID
HalIdtInstallHandler(
	__in UCHAR Number,
	__in VOID( *InterruptHandler )( PKTRAP_FRAME, PKPCR )
);


typedef struct _IDTR {
	USHORT	Limit;
	ULONG64 Base;
} IDTR, *PIDTR;

typedef struct _GDTR {
	USHORT	Limit;
	ULONG64 Base;
} GDTR, *PGDTR;

typedef struct _TSS {
	ULONG32 Reserved0;
	ULONG64 Rsp0;
	ULONG64 Rsp1;
	ULONG64 Rsp2;
	ULONG64 Reserved1;
	ULONG64 Ist1;
	ULONG64 Ist2;
	ULONG64 Ist3;
	ULONG64 Ist4;
	ULONG64 Ist5;
	ULONG64 Ist6;
	ULONG64 Ist7;
	ULONG64 Reserved2;
	ULONG32	IopbOffset;
} TSS, *PTSS;

#define LOCAL_APIC_ID_REGISTER										0x20
#define LOCAL_APIC_VERSION_REGISTER									0x30
#define LOCAL_APIC_TASK_PRIORITY_REGISTER							0x80
#define LOCAL_APIC_ARBITRATION_PRIORITY_REGISTER					0x90
#define LOCAL_APIC_PROCESSOR_PRIORITY_REGISTER						0xA0
#define LOCAL_APIC_END_OF_INTERRUPT_REGISTER						0xB0
#define LOCAL_APIC_REMOTE_READ_REGISTER								0xC0
#define LOCAL_APIC_LOGICAL_DESTINATION_REGISTER						0xD0
#define LOCAL_APIC_DESTINATION_FORMAT_REGISTER						0xE0
#define LOCAL_APIC_SPURIOUS_INTERRUPT_VECTOR_REGISTER				0xF0
#define LOCAL_APIC_IN_SERVICE_REGISTER(x)							0x100+x //0x100 - 0x170
#define LOCAL_APIC_TRIGGER_MODE_REGISTER(x)							0x180+x //0x180 - 0x1f0
#define LOCAL_APIC_INTERRUPT_REQUEST_REGISTER(x)					0x200+x //0x200 - 0x270
#define LOCAL_APIC_ERROR_STATUS_REGISTER							0x280
#define LOCAL_APIC_LVT_CMCI_REGISTER								0x2f0
#define LOCAL_APIC_INTERRUPT_COMMAND_REGISTER(x)					0x300+(x*0x10) //0x300 - 0x310
#define LOCAL_APIC_LVT_TIMER_REGISTER								0x320
#define LOCAL_APIC_LVT_THERMAL_SENSOR_REGISTER						0x330
#define LOCAL_APIC_LVT_PERFORMANCE_MONITORING_COUNTERS_REGISTER		0x340
#define LOCAL_APIC_LVT_LINT0_REGISTER								0x350
#define LOCAL_APIC_LVT_LINT1_REGISTER								0x360
#define LOCAL_APIC_LVT_ERROR_REGISTER								0x370
#define LOCAL_APIC_INITIAL_COUNT_REGISTER							0x380
#define LOCAL_APIC_CURRENT_COUNT_REGISTER							0x390
#define LOCAL_APIC_DIVIDE_CONFIG_REGISTER							0x3e0

#define LOCAL_APIC_CR0_DEST_NORMAL					(0<<8)
#define LOCAL_APIC_CR0_DEST_LOW_PRIORITY			(1<<8)
#define LOCAL_APIC_CR0_DEST_SMI						(2<<8)
#define LOCAL_APIC_CR0_DEST_NMI						(4<<8)
#define LOCAL_APIC_CR0_DEST_INIT_OR_INIT_DEASSERT	(5<<8)
#define LOCAL_APIC_CR0_DEST_SIPI					(6<<8)

#define LOCAL_APIC_CR0_DEST_DISABLE					0x10000

#define LOCAL_APIC_CR0_INIT_DEASSERT				(1<<15)//~(1<<14)
#define LOCAL_APIC_CR0_NO_INIT_DEASSERT				(1<<14)//~(1<<15)

NTSYSAPI
VOID
HalLocalApicSendIpi(
	__in ULONG32 CpuApciId,
	__in ULONG32 Cr0Flags
);
