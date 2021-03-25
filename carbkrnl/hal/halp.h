


#pragma once

#pragma pack(push, 1)

#define HAL_TAG ' LAH'

#define GDT_KERNEL_CODE64   0x08
#define GDT_KERNEL_DATA     0x10
#define GDT_USER_CODE64     0x20
#define GDT_USER_DATA       0x18

#define IDT_GATE_TYPE_INTERRUPT64   0b1110
#define IDT_GATE_TYPE_TRAP64        0b1111

typedef struct _KSEG_DESC_REG {
    USHORT  Limit;
    ULONG64 Base;
} KSEG_DESC_REG, *PKSEG_DESC_REG;

C_ASSERT( sizeof( KSEG_DESC_REG ) == 10 );

typedef union _KGDT_SEG_ENTRY {
    struct {
        ULONG64 LimitLow : 16;
        ULONG64 BaseLow : 16;
        ULONG64 BaseMid : 8;
        ULONG64 Accessed : 1;
        ULONG64 Writeable : 1;
        ULONG64 Direction : 1;
        ULONG64 Executable : 1;
        ULONG64 DescriptorType : 1;
        ULONG64 PrivilegeLevel : 2;
        ULONG64 Present : 1;
        ULONG64 LimitHigh : 4;
        ULONG64 System : 1;
        ULONG64 LongMode : 1;
        ULONG64 DefaultBig : 1;
        ULONG64 Granularity : 1;
        ULONG64 BaseHigh : 8;
    };

    ULONG64     Long;
} KGDT_SEG_ENTRY, *PKGDT_SEG_ENTRY;

C_ASSERT( sizeof( KGDT_SEG_ENTRY ) == 8 );

//TSS or LDT
typedef union _KGDT_ALT_ENTRY {
    struct {
        ULONG64 LimitLow : 16;
        ULONG64 BaseLow : 16;
        ULONG64 BaseMid : 8;
        ULONG64 Accessed : 1;
        ULONG64 Writeable : 1;
        ULONG64 Direction : 1;
        ULONG64 Executable : 1;
        ULONG64 System : 1;
        ULONG64 PrivilegeLevel : 2;
        ULONG64 Present : 1;
        ULONG64 LimitHigh : 4;
        ULONG64 Avail : 1;
        ULONG64 LongMode : 1;
        ULONG64 DefaultBig : 1;
        ULONG64 Granularity : 1;
        ULONG64 BaseHigh : 8;
        ULONG64 BaseUpper : 32;
        ULONG64 Reserved : 32;
    };
    struct {
        ULONG64 Value0;
        ULONG64 Value1;
    };
} KGDT_ALT_ENTRY, *PKGDT_ALT_ENTRY;

C_ASSERT( sizeof( KGDT_ALT_ENTRY ) == 16 );

typedef struct _KTASK_STATE {
    ULONG32 Reserved1;
    ULONG64 Rsp0;
    ULONG64 Rsp1;
    ULONG64 Rsp2;
    ULONG64 Reserved2;
    ULONG64 Ist[ 7 ];
    ULONG64 Reserved3;
    ULONG32 IopbOffset;
} KTASK_STATE, *PKTASK_STATE;

C_ASSERT( sizeof( KTASK_STATE ) == 0x68 );

typedef union _KIDT_GATE {
    struct {
        ULONG64 OffsetLow : 16;
        ULONG64 CodeSelector : 16;
        ULONG64 Ist : 3;
        ULONG64 System : 5;
        ULONG64 Type : 4;
        ULONG64 Zero2 : 1;
        ULONG64 PrivilegeLevel : 2;
        ULONG64 Present : 1;
        ULONG64 OffsetMid : 16;

        ULONG64 OffsetHigh : 32;
        ULONG64 Zero3 : 32;
    };
    struct {
        ULONG64 Value0;
        ULONG64 Value1;
    };
} KIDT_GATE, *PKIDT_GATE;

C_ASSERT( sizeof( KIDT_GATE ) == 16 );

typedef UCHAR KX86_REG[ 10 ];
typedef UCHAR KXMM_REG[ 16 ];

typedef union _KMMX_REG {
    struct {
        KX86_REG St;
        UCHAR    StReserved[ 6 ];
    };
    struct {
        UCHAR    MmValue[ 8 ];
        UCHAR    MmReserved[ 8 ];
    };
} KMMX_REG, *PKMMX_REG;

typedef struct _KFXSAVE64 {
    USHORT      Control;
    USHORT      Status;
    UCHAR       Tag;
    UCHAR       Reserved1;
    USHORT      Opcode;
    ULONG64     Ip64;
    ULONG64     Dp64;
    ULONG32     Mxcsr;
    ULONG32     MxcsrMask;
    KMMX_REG    MmxReg[ 8 ];
    KXMM_REG    XmmReg[ 16 ];
    UCHAR       Reserved2[ 48 ];
    UCHAR       Available[ 48 ];
} KFXSAVE64, *PKFXSAVE64;

C_ASSERT( sizeof( KFXSAVE64 ) == 512 );

typedef struct _KTRAP_FRAME {
    KFXSAVE64 Fxsave64;
    ULONG64   Align2;
    ULONG64   Align1;
    ULONG64   Align0;
    ULONG64   Dr7;
    ULONG64   Dr6;
    ULONG64   Dr3;
    ULONG64   Dr2;
    ULONG64   Dr1;
    ULONG64   Dr0;
    ULONG64   Cr3;
    ULONG64   SegGs;
    ULONG64   SegFs;
    ULONG64   SegEs;
    ULONG64   SegDs;
    ULONG64   R15;
    ULONG64   R14;
    ULONG64   R13;
    ULONG64   R12;
    ULONG64   R11;
    ULONG64   R10;
    ULONG64   R9;
    ULONG64   R8;
    ULONG64   Rdi;
    ULONG64   Rsi;
    ULONG64   Rbp;
    ULONG64   Rbx;
    ULONG64   Rdx;
    ULONG64   Rcx;
    ULONG64   Rax;
    ULONG64   Interrupt;
    ULONG64   Error;
    ULONG64   Rip;
    ULONG64   SegCs;
    ULONG64   EFlags;
    ULONG64   Rsp;
    ULONG64   SegSs;

} KTRAP_FRAME, *PKTRAP_FRAME;

C_ASSERT( ( sizeof( KTRAP_FRAME ) - 512 ) % 0x20 == 0 );

typedef struct _KSTARTUP {
    ULONG32 TableBase;
    USHORT  GdtLimit;
    ULONG32 GdtBase;
    ULONG64 InitialRsp;
    ULONG64 InitialJmp;
} KSTARTUP, *PKSTARTUP;

EXTERN
VOID
HalProcessorStartup(

);

EXTERN
VOID
HalProcessorStartupEnd(

);

VOID
HalStartProcessors(

);

#define LAPIC_ID_REGISTER                                      0x20
#define LAPIC_VERSION_REGISTER                                 0x30
#define LAPIC_TASK_PRIORITY_REGISTER                           0x80
#define LAPIC_ARBITRATION_PRIORITY_REGISTER                    0x90
#define LAPIC_PROCESSOR_PRIORITY_REGISTER                      0xA0
#define LAPIC_END_OF_INTERRUPT_REGISTER                        0xB0
#define LAPIC_REMOTE_READ_REGISTER                             0xC0
#define LAPIC_LOGICAL_DESTINATION_REGISTER                     0xD0
#define LAPIC_DESTINATION_FORMAT_REGISTER                      0xE0
#define LAPIC_SPURIOUS_INTERRUPT_VECTOR_REGISTER               0xF0
#define LAPIC_IN_SERVICE_REGISTER(x)                           0x100+x //0x100 - 0x170
#define LAPIC_TRIGGER_MODE_REGISTER(x)                         0x180+x //0x180 - 0x1f0
#define LAPIC_INTERRUPT_REQUEST_REGISTER(x)                    0x200+x //0x200 - 0x270
#define LAPIC_ERROR_STATUS_REGISTER                            0x280
#define LAPIC_LVT_CMCI_REGISTER                                0x2f0
#define LAPIC_INTERRUPT_COMMAND_REGISTER(x)                    0x300+(x*0x10) //0x300 - 0x310
#define LAPIC_LVT_TIMER_REGISTER                               0x320
#define LAPIC_LVT_THERMAL_SENSOR_REGISTER                      0x330
#define LAPIC_LVT_PERFORMANCE_MONITORING_COUNTERS_REGISTER     0x340
#define LAPIC_LVT_LINT0_REGISTER                               0x350
#define LAPIC_LVT_LINT1_REGISTER                               0x360
#define LAPIC_LVT_ERROR_REGISTER                               0x370
#define LAPIC_INITIAL_COUNT_REGISTER                           0x380
#define LAPIC_CURRENT_COUNT_REGISTER                           0x390
#define LAPIC_DIVIDE_CONFIG_REGISTER                           0x3e0

#define TIMER_ONE_SHOT                  ( 0 << 17 )
#define TIMER_PERIODIC                  ( 1 << 17 )
#define TIMER_TSC_DEADLINE              ( 2 << 17 )
#define TIMER_MODE                      ( 3 << 17 )

#define LAPIC_MASKED                    ( 1 << 16 )
#define LAPIC_L                         ( 1 << 14 )
#define LAPIC_L_ASSERT                  ( 1 << 14 ) 
#define LAPIC_L_DEASSERT                ( 0 << 14 ) 
#define LAPIC_TGM_LEVEL                 ( 1 << 15 )
#define LAPIC_TGM_EDGE                  ( 0 << 15 ) 
#define LAPIC_DS                        ( 1 << 12 ) // 0=idle, 1=send pending
#define LAPIC_MT                        ( 7 << 8 )
#define LAPIC_MT_FIXED                  ( 0 << 8 )
#define LAPIC_MT_LOWEST_PRIORITY        ( 1 << 8 )
#define LAPIC_MT_SMI                    ( 2 << 8 )
#define LAPIC_MT_NMI                    ( 4 << 8 )
#define LAPIC_MT_INIT                   ( 5 << 8 )
#define LAPIC_MT_START_UP               ( 6 << 8 )
#define LAPIC_DM                        ( 1 << 11 )
#define LAPIC_DM_PHYSICAL               ( 0 << 11 )
#define LAPIC_DM_LOGICAL                ( 1 << 11 )
#define LAPIC_DSH                       ( 3 << 18 )
#define LAPIC_DSH_NONE                  ( 0 << 18 )
#define LAPIC_DSH_SELF                  ( 1 << 18 )
#define LAPIC_DSH_ALL_INCLUDING_SELF    ( 2 << 18 )
#define LAPIC_DSH_ALL_EXCLUDING_SELF    ( 3 << 18 )
#define LAPIC_DES                       ( 15 << 24 )
#define LAPIC_RRS                       ( 3 << 17 ) // 0=read invalid, 1=deliv pending, 2=deliv done, access valid

#define MADT_TABLES_MAX     0x20

typedef struct _RSDP_DESCRIPTOR1_0 {
    CHAR        RsdpSignature[ 8 ];
    UCHAR       Checksum;
    CHAR        OemId[ 6 ];
    UCHAR       Revision;
    ULONG       RsdtAddress;
} RSDP_DESCRIPTOR1_0, *PRSDP_DESCRIPTOR1_0;

typedef struct _RSDP_DESCRIPTOR2_0 {
    RSDP_DESCRIPTOR1_0  Rsdp1_0;

    ULONG               Length;
    ULONG64             XsdtAddress;
    UCHAR               ExtendedChecksum;
    UCHAR               Reserved[ 3 ];
} RSDP_DESCRIPTOR2_0, *PRSDP_DESCRIPTOR2_0;

typedef struct _SDT_HEADER {
    CHAR        Signature[ 4 ];
    ULONG       Length;
    UCHAR       Revision;
    UCHAR       Checksum;
    CHAR        OemIdentifier[ 6 ];
    CHAR        OemTableId[ 8 ];
    ULONG       OemRevision;
    ULONG       CreatorId;
    ULONG       CreatorRevision;
} SDT_HEADER, *PSDT_HEADER;

typedef struct _XSDT_DESCRIPTOR {
    SDT_HEADER  Header;
    ULONG64     SdtPointers[ 1 ];
} XSDT_DESCRIPTOR, *PXSDT_DESCRIPTOR;

typedef struct _RSDT_DESCRIPTOR {
    SDT_HEADER  Header;
    ULONG32     SdtPointers[ 1 ];
} RSDT_DESCRIPTOR, *PRSDT_DESCRIPTOR;

#define ACPI_MADT_TYPE_PROCESSOR_LOCAL_APIC             0
#define ACPI_MADT_TYPE_IO_APIC                          1
#define ACPI_MADT_TYPE_INTERRUPT_SOURCE_OVERRIDE_APIC   2
#define ACPI_MADT_TYPE_NON_MASKABLE_INTERRUPT_APIC      4
#define ACPI_MADT_TYPE_LOCAL_APIC_ADDRESS_OVERRIDE_APIC 5

typedef struct _MADT_HEADER {
    UCHAR       EntryType;
    UCHAR       RecordLength;
} MADT_HEADER, *PMADT_HEADER;

typedef struct _MADT {
    SDT_HEADER  Header;
    ULONG       LocalApicAddress;
    ULONG       Flags;
    UCHAR       Entry0;
} MADT, *PMADT;

typedef struct _MADT_PROCESSOR_LOCAL_APIC {
    MADT_HEADER Header;
    UCHAR       AcpiProcessorId;
    UCHAR       ApicId;
    ULONG       Flags;
} MADT_PROCESSOR_LOCAL_APIC, *PMADT_PROCESSOR_LOCAL_APIC;

typedef struct _MADT_IO_APIC {
    MADT_HEADER Header;
    UCHAR       IoApicId;
    UCHAR       Reserved;
    ULONG       IoApicAddress;
    ULONG       GlobalSystemInterruptBase;
} MADT_IO_APIC, *PMADT_IO_APIC;

typedef struct _MADT_INTERRUPT_SOURCE_OVERRIDE_APIC {
    MADT_HEADER Header;
    UCHAR       BusSource;
    UCHAR       IrqSource;
    ULONG       GlobalSystemInterrupt;
    USHORT      Flags;
} MADT_INTERRUPT_SOURCE_OVERRIDE_APIC, *PMADT_INTERRUPT_SOURCE_OVERRIDE_APIC;

typedef struct _MADT_NON_MASKABLE_INTERRUPT_APIC {
    MADT_HEADER Header;
    UCHAR       AcpiProcessorId;//0xff = all.
    USHORT      Flags;
    UCHAR       LocalInterrupt;
} MADT_NON_MASKABLE_INTERRUPT_APIC, *PMADT_NON_MASKABLE_INTERRUPT_APIC;

typedef struct _MADT_LOCAL_APIC_ADDRESS_OVERRIDE {
    MADT_HEADER Header;
    USHORT      Reserved;
    ULONG64     LocalApicAddress;
} MADT_LOCAL_APIC_ADDRESS_OVERRIDE, *PMADT_LOCAL_APIC_ADDRESS_OVERRIDE;

EXTERN PRSDP_DESCRIPTOR2_0                      HalRsdp;
EXTERN PRSDT_DESCRIPTOR                         HalRsdt;
EXTERN PXSDT_DESCRIPTOR                         HalXsdt;
EXTERN PMADT                                    HalMadt;

EXTERN PMADT_PROCESSOR_LOCAL_APIC*              HalLocalApics;
EXTERN PMADT_IO_APIC*                           HalIoApics;
EXTERN PMADT_INTERRUPT_SOURCE_OVERRIDE_APIC*    HalIsoApics;
EXTERN PMADT_NON_MASKABLE_INTERRUPT_APIC*       HalNmiApics;

EXTERN ULONG                                    HalLocalApicCount;
EXTERN ULONG                                    HalIoApicCount;
EXTERN ULONG                                    HalIsoApicCount;
EXTERN ULONG                                    HalNmiApicCount;

EXTERN PMADT_LOCAL_APIC_ADDRESS_OVERRIDE        HalLocalApicAddressOverride;

EXTERN PUCHAR                                   HalLocalApic;
EXTERN PUCHAR                                   HalIoApic;

#define IA32_MSR_APIC_BASE              0x0000001B
#define IA32_MSR_FS_BASE                0xC0000100
#define IA32_MSR_GS_BASE                0xC0000101
#define IA32_MSR_GS_KERNEL_BASE         0xC0000102
#define IA32_MSR_EFER                   0xC0000080
#define IA32_MSR_STAR                   0xC0000081
#define IA32_MSR_LSTAR                  0xC0000082
#define IA32_MSR_SFMASK                 0xC0000084

FORCEINLINE
ULONG32
HalIoApicRead(
    _In_ ULONG64 IoApicAddress,
    _In_ ULONG32 Register
)
{
    ULONG32* IoApic = ( ULONG32* )IoApicAddress;
    IoApic[ 0 ] = ( Register & 0xff );

    return IoApic[ 4 ];
}

FORCEINLINE
VOID
HalIoApicWrite(
    _In_ ULONG64 IoApicAddress,
    _In_ ULONG32 Register,
    _In_ ULONG32 Value
)
{
    ULONG32* IoApic = ( ULONG32* )IoApicAddress;
    IoApic[ 0 ] = ( Register & 0xff );
    IoApic[ 4 ] = Value;

    return;
}

FORCEINLINE
ULONG32
HalLocalApicRead(
    _In_ ULONG32 Register
)
{
    return *( VOLATILE ULONG32* )( HalLocalApic + Register );
}

FORCEINLINE
VOID
HalLocalApicWrite(
    _In_ ULONG32 Register,
    _In_ ULONG32 Value
)
{
    //RtlDebugPrint( L"Base: %ULL, Apic: %ULL\n", HalLocalApic, ( ULONG32* )( ( PCHAR )HalLocalApic + Register ) );
    *( VOLATILE ULONG32* )( HalLocalApic + Register ) = Value;
}

FORCEINLINE
VOID
HalLocalApicSendIpi(
    _In_ ULONG32 ApicId,
    _In_ ULONG32 Flags
)
{
    //RtlDebugPrint( L"Sending ipi to %d %ul\n", ApicId, Flags & 0xFF );
    HalLocalApicWrite( LAPIC_INTERRUPT_COMMAND_REGISTER( 1 ), ApicId << 24 );
    HalLocalApicWrite( LAPIC_INTERRUPT_COMMAND_REGISTER( 0 ), Flags );
}

#define PIT_IOPORT_CHANNEL0_DATA 0x40
#define PIT_IOPORT_CHANNEL1_DATA 0x41
#define PIT_IOPORT_CHANNEL2_DATA 0x42
#define PIT_IOPORT_MODE_REGISTER 0x43

FORCEINLINE
VOID
HalDelayExecutionPit(
    _In_ USHORT Milliseconds
)
{
    USHORT Divisor;

    while ( Milliseconds > 32 ) {

        Divisor = ( USHORT )( ( 1193181 / 1000 ) * 32 );
        __outbyte( PIT_IOPORT_MODE_REGISTER, 0x30 );
        __outbyte( PIT_IOPORT_CHANNEL0_DATA, ( UCHAR )( Divisor ) );
        __outbyte( PIT_IOPORT_CHANNEL0_DATA, ( UCHAR )( Divisor >> 8 ) );

        while ( 1 ) {

            __outbyte( PIT_IOPORT_MODE_REGISTER, 0xE2 );
            if ( __inbyte( PIT_IOPORT_CHANNEL0_DATA ) & ( 1 << 7 ) ) {

                break;
            }
        }
        Milliseconds -= 32;
    }

    if ( Milliseconds != 0 ) {
        Divisor = ( USHORT )( ( 1193181 / 1000 ) * Milliseconds );
        __outbyte( PIT_IOPORT_MODE_REGISTER, 0x30 );
        __outbyte( PIT_IOPORT_CHANNEL0_DATA, ( UCHAR )( Divisor ) );
        __outbyte( PIT_IOPORT_CHANNEL0_DATA, ( UCHAR )( Divisor >> 8 ) );

        while ( 1 ) {

            __outbyte( PIT_IOPORT_MODE_REGISTER, 0xE2 );
            if ( __inbyte( PIT_IOPORT_CHANNEL0_DATA ) & ( 1 << 7 ) ) {

                break;
            }
        }
    }
}

VOID
HalInitializeIdt(
    _Inout_ PKIDT_GATE     Table,
    _Out_   PKSEG_DESC_REG Idtr
);

VOID
HalInitializeCpu0(

);

VOID
HalGdtCreate(
    _Inout_ PKSEG_DESC_REG Gdtr
);

ULONG
HalGdtAddSegEntry(
    _Inout_ PKSEG_DESC_REG  Gdtr,
    _In_    PKGDT_SEG_ENTRY Entry
);

ULONG
HalGdtAddAltEntry(
    _Inout_ PKSEG_DESC_REG  Gdtr,
    _In_    PKGDT_ALT_ENTRY Entry
);

ULONG
HalGdtAddTss(
    _Inout_ PKSEG_DESC_REG Gdtr,
    _In_    PKTASK_STATE   Task,
    _In_    ULONG          Length
);

VOID
HalGdtSetSegBase(
    _Inout_ PKGDT_SEG_ENTRY Entry,
    _In_    PVOID           Base
);

void
__ltr(
    _In_ unsigned short tr
);

unsigned short
__str(

);

void
__interrupt(
    _In_ unsigned char vector
);

VOID
HalInitializeAcpi(

);

VOID
HalLocalApicEnable(

);

VOID
HalProcessorStartupPrepare(

);

VOID
HalEnableCpuFeatures(

);

VOID
HalEoi(
    _In_ ULONG64 Vector
);

#pragma pack(pop)
