/*++

Module ObjectName:

	hal.h

Abstract:

	.

--*/

#pragma once

#define PIC_MASTER_IOPORT_COMMAND   0x20
#define PIC_MASTER_IOPORT_DATA      0x21
#define PIC_SLAVE_IOPORT_COMMAND    0xa0
#define PIC_SLAVE_IOPORT_DATA       0xa1
#define PIC_EOI_COMMAND             0x20

#define ICW1_ICW4	    0x01		/* ICW4 (not) needed */
#define ICW1_SINGLE	    0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL	    0x08		/* Level triggered (edge) mode */
#define ICW1_INIT	    0x10		/* Initialization - required! */

#define ICW4_8086	    0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	    0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
#define ICW4_SFNM	    0x10		/* Special fully nested (not) */

VOID
HalPic8259aSetIrqMasks(
	__in UCHAR MasterMask,
	__in UCHAR SlaveMask
);

VOID
HalPic8259aGetIrqMasks(
	__out PUCHAR MasterMask,
	__out PUCHAR SlaveMask
);

VOID
HalPic8259aRemapVectorOffsets(
	__in UCHAR MasterOffset,
	__in UCHAR SlaveOffset
);

void
__interrupt(
	__in unsigned char vector
);

void __fninit( );

#define HalClearInterruptFlag() __writeeflags(__readeflags()&~0x200)
#define HalSetInterruptFlag() __writeeflags(__readeflags()|0x200)

VOID
HalPciEnumerate(

);

#define CPUID_FEATURE_EDX_APIC		(1 << 9)

#define MSR_APIC_BASE			0x1B
//bit flags we dont even use.
//#define MSR_APIC_BASE_BSP		0x100
//#define MSR_APIC_BASE_ENABLE	0x800
#define MSR_FS_BASE				0xC0000100
#define MSR_GS_BASE				0xC0000101
#define MSR_GS_KERNEL_BASE		0xC0000102
#define MSR_PAT					0x00000277

/*
PAT_UNCACHEABLE    = 0x00
PAT_WRITECOMBINING = 0x01
PAT_WRITETHROUGH   = 0x04
PAT_WRITEPROTECTED = 0x05
PAT_WRITEBACK      = 0x06
PAT_UNCACHED       = 0x07
*/

#define PAT_UNCACHEABLE		0x00
#define PAT_WRITECOMBINING	0x01
#define PAT_WRITETHROUGH	0x04
#define PAT_WRITEPROTECTED	0x05
#define PAT_WRITEBACK		0x06
#define PAT_UNCACHED		0x07

VOID
HalApicSetLocalBaseMsr(
	__in ULONG64 ApicBase
);

ULONG64
HalApicGetLocalBaseMsr(

);

ULONG32
HalLocalApicRead(
	__in ULONG32 Register
);

VOID
HalLocalApicWrite(
	__in ULONG32 Register,
	__in ULONG32 Value
);

VOID
HalLocalApicEnable(

);

VOID
HalLocalApicTimerEnable(

);

VOID
HalSmpTrampolineStart(

);

VOID
HalSmpEnableCpuFeatures(

);

VOID
HalSmpInitializeCpu0(

);

VOID
HalSmpInitializeCpu(

);

#define IDT_GATE_TYPE_TASK32        0b0101
#define IDT_GATE_TYPE_INTERRUPT16   0b0110
#define IDT_GATE_TYPE_TRAP16        0b0111
#define IDT_GATE_TYPE_INTERRUPT32   0b1110
#define IDT_GATE_TYPE_TRAP32        0b1111

typedef struct _INTERRUPT_DESCRIPTOR_TABLE {
	USHORT	Offset0;
	USHORT	CodeSelector;
	UCHAR	InterruptStackTable;
	UCHAR	TypeAttribute;
	USHORT	Offset1;
	ULONG32	Offset2;
	ULONG32	Zero;
} INTERRUPT_DESCRIPTOR_TABLE, *PINTERRUPT_DESCRIPTOR_TABLE;

EXTERN ULONG64 HalInterruptHandlerTable[ ];

VOID
HalIdtInitialize(
	__inout PINTERRUPT_DESCRIPTOR_TABLE DescriptorTable,
	__inout PIDTR Idtr
);

#define GDT_KERNEL_CODE64	0x8
#define GDT_KERNEL_DATA		0x10
#define GDT_USER_CODE64     0x20
#define GDT_USER_DATA       0x18

typedef struct _GLOBAL_DESCRIPTOR_TABLE_ENTRY {
	USHORT LimitLow;
	USHORT BaseLow;
	UCHAR BaseMid;
	//UCHAR AccessByte;

	struct {
		UCHAR Accessed : 1;
		UCHAR Writeable : 1;
		UCHAR DirectionConforming : 1;
		UCHAR Executable : 1;
		UCHAR DescriptorType : 1;
		UCHAR PrivilegeLevel : 2;
		UCHAR PresentBit : 1;
	} AccessByte;

	//UCHAR LimitHigh : 4;
	//UCHAR Flags : 4;

	struct {
		UCHAR LimitHigh : 4;

		UCHAR Reserved : 1;

		UCHAR CodeLongMode : 1; //data = reserved
		UCHAR CodeProtectedMode : 1; //0 if CodeLongMode = 1

		//UCHAR CodeExecutionMode : 2; //2 = 64, 1 = 32, 0 = 16, 3 = gpf.
		UCHAR Granularity : 1;
	} Flags;

	UCHAR BaseHigh;
} GLOBAL_DESCRIPTOR_TABLE_ENTRY, *PGLOBAL_DESCRIPTOR_TABLE_ENTRY;

typedef struct _GDT_ENTRY_TSS {
	USHORT Length;
	USHORT BaseLow;
	UCHAR BaseMid;
	UCHAR AccessByte;
	UCHAR Flags;//and limit but we dont need it.
	UCHAR BaseHigh;
	ULONG32 BaseHigh32;
	ULONG32 Reserved;
} GDT_ENTRY_TSS, *PGDT_ENTRY_TSS;

VOID
HalGdtCreate(
	__inout PGDTR Gdtr
);

USHORT
HalGdtAddEntry(
	__inout PGDTR Gdtr,
	__in PGLOBAL_DESCRIPTOR_TABLE_ENTRY GdtEntry
);

void
__ltr(
	__in unsigned short tr
);

unsigned short
__str(

);

#define KERNEL_STACK_SIZE 0x4000

USHORT
HalGdtAddTss(
	__inout PGDTR Gdtr,
	__in PTSS Address,
	__in USHORT Size
);

#define PIT_IOPORT_CHANNEL0_DATA 0x40
#define PIT_IOPORT_CHANNEL1_DATA 0x41
#define PIT_IOPORT_CHANNEL2_DATA 0x42
#define PIT_IOPORT_MODE_REGISTER 0x43

VOID
HalPitDelay(
	__in USHORT Milliseconds
);

