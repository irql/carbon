/*++

Module ObjectName:

	acpi.h

Abstract:

	Defines data structures and procedure prototypes for ACPI.

--*/

#pragma once

#define RSDP_SIGNATURE		"RSD PTR "
#define MADT_TABLES_MAX		0x20

typedef struct _RSDP_DESCRIPTOR1_0 {
	CHAR		RsdpSignature[8];
	UCHAR		Checksum;
	CHAR		OemId[6];
	UCHAR		Revision;
	ULONG		RsdtAddress;
} RSDP_DESCRIPTOR1_0, *PRSDP_DESCRIPTOR1_0;

typedef struct _RSDP_DESCRIPTOR2_0 {
	RSDP_DESCRIPTOR1_0	Rsdp1_0;

	ULONG				Length;
	ULONG64				XsdtAddress;
	UCHAR				ExtendedChecksum;
	UCHAR				Reserved[3];
} RSDP_DESCRIPTOR2_0, *PRSDP_DESCRIPTOR2_0;

typedef struct _SDT_HEADER {
	CHAR		Signature[4];
	ULONG		Length;
	UCHAR		Revision;
	UCHAR		Checksum;
	CHAR		OemIdentifier[6];
	CHAR		OemTableId[8];
	ULONG		OemRevision;
	ULONG		CreatorId;
	ULONG		CreatorRevision;
} SDT_HEADER, *PSDT_HEADER;

typedef struct _XSDT_DESCRIPTOR {
	SDT_HEADER	Header;
	ULONG64		SdtPointers[1];
} XSDT_DESCRIPTOR, *PXSDT_DESCRIPTOR;

typedef struct _RSDT_DESCRIPTOR {
	SDT_HEADER	Header;
	ULONG32		SdtPointers[1];
} RSDT_DESCRIPTOR, *PRSDT_DESCRIPTOR;

#define ACPI_MADT_TYPE_PROCESSOR_LOCAL_APIC				0
#define ACPI_MADT_TYPE_IO_APIC							1
#define ACPI_MADT_TYPE_INTERRUPT_SOURCE_OVERRIDE_APIC	2
#define ACPI_MADT_TYPE_NON_MASKABLE_INTERRUPT_APIC		4
#define ACPI_MADT_TYPE_LOCAL_APIC_ADDRESS_OVERRIDE_APIC 5

typedef struct _MADT_HEADER {
	UCHAR		EntryType;
	UCHAR		RecordLength;
} MADT_HEADER, *PMADT_HEADER;

typedef struct _MADT {
	SDT_HEADER	Header;
	ULONG		LocalApicAddress;
	ULONG		Flags;
	UCHAR		Entry0;
} MADT, *PMADT;

typedef struct _MADT_PROCESSOR_LOCAL_APIC {
	MADT_HEADER Header;
	UCHAR		AcpiProcessorId;
	UCHAR		ApicId;
	ULONG		Flags;
} MADT_PROCESSOR_LOCAL_APIC, *PMADT_PROCESSOR_LOCAL_APIC;

typedef struct _MADT_IO_APIC {
	MADT_HEADER Header;
	UCHAR		IoApicId;
	UCHAR		Reserved;
	ULONG		IoApicAddress;
	ULONG		GlobalSystemInterruptBase;
} MADT_IO_APIC, *PMADT_IO_APIC;

typedef struct _MADT_INTERRUPT_SOURCE_OVERRIDE_APIC {
	MADT_HEADER Header;
	UCHAR		BusSource;
	UCHAR		IrqSource;
	ULONG		GlobalSystemInterrupt;
	USHORT		Flags;
} MADT_INTERRUPT_SOURCE_OVERRIDE_APIC, *PMADT_INTERRUPT_SOURCE_OVERRIDE_APIC;

typedef struct _MADT_NON_MASKABLE_INTERRUPT_APIC {
	MADT_HEADER Header;
	UCHAR		AcpiProcessorId;//0xff = all.
	USHORT		Flags;
	UCHAR		LocalInterrupt;
} MADT_NON_MASKABLE_INTERRUPT_APIC, *PMADT_NON_MASKABLE_INTERRUPT_APIC;

typedef struct _MADT_LOCAL_APIC_ADDRESS_OVERRIDE {
	MADT_HEADER Header;
	USHORT		Reserved;
	ULONG64		LocalApicAddress;
} MADT_LOCAL_APIC_ADDRESS_OVERRIDE, *PMADT_LOCAL_APIC_ADDRESS_OVERRIDE;

EXTERN PRSDP_DESCRIPTOR2_0						Rsdp;
EXTERN PRSDT_DESCRIPTOR							Rsdt;
EXTERN PXSDT_DESCRIPTOR							Xsdt;
EXTERN PMADT									Madt;

EXTERN PMADT_PROCESSOR_LOCAL_APIC*				MadtProcessorLocalApics;
EXTERN PMADT_IO_APIC*							MadtIoApics;
EXTERN PMADT_INTERRUPT_SOURCE_OVERRIDE_APIC*	MadtInterruptSourceOverrideApics;
EXTERN PMADT_NON_MASKABLE_INTERRUPT_APIC*		MadtNonMaskableInterruptApics;

EXTERN ULONG									MadtProcessorLocalApicCount;
EXTERN ULONG									MadtIoApicCount;
EXTERN ULONG									MadtInterruptSourceOverrideApicCount;
EXTERN ULONG									MadtNonMaskableInterruptApicCount;

EXTERN PMADT_LOCAL_APIC_ADDRESS_OVERRIDE		MadtLocalApicAddressOverride;

VOID
HalInitializeAcpi(

	);


PSDT_HEADER
HalAcpiFindSdt(
	__in PCHAR Signature
	);