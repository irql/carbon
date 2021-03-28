


#pragma once

#define KE_TAG 'eroC'

#define THREAD_STATE_IDLE           0x00
#define THREAD_STATE_RUNNING        0x01
#define THREAD_STATE_READY          0x02
#define THREAD_STATE_NOT_READY      0x03
#define THREAD_STATE_TERMINATING    0x04
#define THREAD_STATE_TERMINATED     0x05
#define THREAD_STATE_WAITING        0x06

typedef struct _KAPC {
    int x;
} KAPC, *PKAPC;

typedef struct _KTHREAD {
    KDPC_HEADER Header;
#pragma pack(push, 1)
    struct {
        ULONG32 ServiceNumber;
        ULONG32 PreviousEFlags;
        ULONG64 PreviousIp;
        ULONG64 PreviousStack;
    } SystemCall;
#pragma pack(pop)

    ULONG64     KernelStackBase;
    ULONG64     KernelStackLength;

    ULONG64     StackBase;
    ULONG64     StackLength;

    KTRAP_FRAME TrapFrame;
    LIST_ENTRY  ThreadQueue;
    ULONG64     ThreadState;

    ULONG64     ProcessorNumber;
    ULONG64     ThreadId;
    ULONG64     ExitCode;

    PKPROCESS   Process;
    LIST_ENTRY  ThreadLinks;

    PVOID       WaitObject;
    ULONG64     WaitTimeout;
    ULONG64     Reserved0;

    ULONG64     SuspendCount;
    KSPIN_LOCK  ThreadLock;

    KPROCESSOR_MODE PreviousMode;
    BOOLEAN     Syscall;

} KTHREAD, *PKTHREAD;

typedef struct _MM_VAD {
    PMM_VAD         Link;
    ULONG64         Start;
    ULONG64         End;
    ULONG64         Charge;
    PIO_FILE_OBJECT FileObject;
} MM_VAD, *PMM_VAD;

typedef struct _KPROCESS {

    ULONG64      ThreadCount;
    PLIST_ENTRY  ThreadLinks;
    HANDLE_TABLE HandleTable;
    ULONG64      DirectoryTableBase;
    ULONG64      UserRegionHint;
    KSPIN_LOCK   WorkingSetLock;
    KSPIN_LOCK   UserRegionLock;
    PMM_VAD      VadRoot;
    LIST_ENTRY   ProcessLinks;
    ULONG64      ProcessId;
    PPEB         Peb;

    KSPIN_LOCK   StackLock;
    PLIST_ENTRY  StackLinks;
    ULONG64      StackCharge;
    ULONG64      StackCount;
} KPROCESS, *PKPROCESS;

#pragma pack(push, 8)

#ifdef KRNLINTERNAL

typedef struct _IO_LOCK_CONTEXT {

    PKSPIN_LOCK Lock;
    ULONG64     PreviousIF;
    KIRQL       PreviousIrql;
    ULONG64     IsHolding;

} IO_LOCK_CONTEXT, *PIO_LOCK_CONTEXT;

typedef struct _KPCB {
    PVOID             Reserved;
    ULONG64           ProcessorNumber;
    ULONG64           ApicId;
    BOOLEAN           InService;
    KDESCRIPTOR_TABLE Global;
    KDESCRIPTOR_TABLE Interrupt;
    ULONG32           TaskStateDescriptor;
    KTASK_STATE       TaskState;

    ULONG64           ThreadQueueLength;
    PKTHREAD          ThreadQueue;
    KSPIN_LOCK        ThreadQueueLock;

    PKDPC             DpcQueue;
    ULONG64           DpcQueueLength;
    KSPIN_LOCK        DpcQueueLock;
    BOOLEAN           SaveThread;
    ULONG64           PreviousService;
    ULONG32           SegGs;

    ULONG64           TickCount;
    IO_LOCK_CONTEXT   LockContext;

    ULONG64           ProcessorFeatures[ ROUND( KPF_MAXIMUM, 64 ) / 64 ];

} KPCB, *PKPCB;

C_ASSERT( FIELD_OFFSET( KPCB, ThreadQueue ) == 168 );
// v

#endif

//
// Macro offsets inside call_gate.asm
//
// If you change the structs and one of these assertions
// fail, then you need to edit the macro in call_gate.asm
//

C_ASSERT( FIELD_OFFSET( KTHREAD, Syscall ) == 0x3d1 );
C_ASSERT( FIELD_OFFSET( KTHREAD, PreviousMode ) == 0x3d0 );
C_ASSERT( FIELD_OFFSET( KTHREAD, SystemCall.ServiceNumber ) == 8 );
C_ASSERT( FIELD_OFFSET( KTHREAD, SystemCall.PreviousEFlags ) == 12 );
C_ASSERT( FIELD_OFFSET( KTHREAD, SystemCall.PreviousIp ) == 16 );
C_ASSERT( FIELD_OFFSET( KTHREAD, SystemCall.PreviousStack ) == 24 );
C_ASSERT( FIELD_OFFSET( KTHREAD, KernelStackBase ) == 32 );
C_ASSERT( FIELD_OFFSET( KTHREAD, KernelStackLength ) == 40 );

#pragma pack(pop)

BOOLEAN
KiTrapDispatcher(
    _In_ PKINTERRUPT Interrupt,
    _In_ PVOID       Context
);

BOOLEAN
KiTrapException(
    _In_ PKINTERRUPT Interrupt
);

PKPCB
KiCreatePcb(

);

VOID
KeInitializeKernelCore(

);

ULONG64
KeGenerateUniqueId(

);

NTSYSAPI // TEMP
VOID
KiLeaveQuantumEarly(

);

BOOLEAN
KiTrapProcessorWakeup(
    _In_ PKINTERRUPT Interrupt
);

VOID
KiSleepIdleProcessor(

);

VOID
KiEnsureProcessorReady(
    _In_ ULONG64 Number
);

VOID
KiEnsureAllProcessorsReady(

);

NTSYSAPI
NTSTATUS
KeInstallServiceDescriptorTable(
    _In_ ULONG32          ServiceTableIndex,
    _In_ ULONG32          ServiceCount,
    _In_ PKSYSTEM_SERVICE ServiceTable
);

VOID
KiInitializeServiceCallTable(

);

EXTERN
VOID
KiFastSystemCall(

);

typedef struct _KIPI_SERVICE {
    PKIPI_CALL BroadcastFunction;
    PVOID      BroadcastContext;

} KIPI_SERVICE, *PKIPI_SERVICE;

VOID
KiInitializeIpiCall(

);


VOID
KeInitializeKernelClock(

);

#define KI_EXCEPTION_DIVIDE_FAULT           0x00
#define KI_EXCEPTION_DEBUG                  0x01
#define KI_EXCEPTION_NMI                    0x02
#define KI_EXCEPTION_BREAKPOINT             0x03
#define KI_EXCEPTION_OVERFLOW               0x04
#define KI_EXCEPTION_BOUND_RANGE            0x05
#define KI_EXCEPTION_INVALID_OP             0x06
#define KI_EXCEPTION_DEVICE_NOT_AVAILABLE   0x07
#define KI_EXCEPTION_DOUBLE_FAULT           0x08
#define KI_EXCEPTION_INVALID_TSS            0x0A
#define KI_EXCEPTION_SEGMENT_NOT_PRESENT    0x0B
#define KI_EXCEPTION_STACK_SEGMENT          0x0C
#define KI_EXCEPTION_GENERAL_PROTECTION     0x0D
#define KI_EXCEPTION_PAGE_FAULT             0x0E
#define KI_EXCEPTION_FPU_EXCEPTION          0x10
#define KI_EXCEPTION_ALIGNMENT_CHECK        0x11
#define KI_EXCEPTION_MACHINE_CHECK          0x12
#define KI_EXCEPTION_SIMD_EXCEPTION         0x13
#define KI_EXCEPTION_VIRTUAL_EXCEPTION      0x14
#define KI_EXCEPTION_SECURITY_EXCEPTION     0x1E
#define KI_EXCEPTION_FAST_FAIL              0x29
#define KI_EXCEPTION_ASSERTION_FAILURE      0x2C

#define E_DE KI_EXCEPTION_DIVIDE_FAULT
#define E_DB KI_EXCEPTION_DEBUG
#define E_BP KI_EXCEPTION_BREAKPOINT
#define E_OF KI_EXCEPTION_OVERFLOW
#define E_BR KI_EXCEPTION_BOUND_RANGE
#define E_UD KI_EXCEPTION_INVALID_OP
#define E_NM KI_EXCEPTION_DEVICE_NOT_AVAILABLE
#define E_DF KI_EXCEPTION_DOUBLE_FAULT
#define E_TS KI_EXCEPTION_INVALID_TSS
#define E_NP KI_EXCEPTION_SEGMENT_NOT_PRESENT
#define E_SS KI_EXCEPTION_STACK_SEGMENT
#define E_GP KI_EXCEPTION_GENERAL_PROTECTION
#define E_PF KI_EXCEPTION_PAGE_FAULT
#define E_MF KI_EXCEPTION_FPU_EXCEPTION
#define E_AC KI_EXCEPTION_ALIGNMENT_CHECK
#define E_XM KI_EXCEPTION_SIMD_EXCEPTION
#define E_XF E_XM
#define E_VE KI_EXCEPTION_VIRTUAL_EXCEPTION
#define E_SX KI_EXCEPTION_SECURITY_EXCEPTION
