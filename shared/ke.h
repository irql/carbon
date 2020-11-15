/*++

Module ObjectName:

	ke.h

Abstract:

	.

--*/

#pragma once

typedef struct _LIST_ENTRY {
	struct _LIST_ENTRY *Flink;
	struct _LIST_ENTRY *Blink;
} LIST_ENTRY, *PLIST_ENTRY;

typedef struct _KSPIN_LOCK {
	VOLATILE PKTHREAD ThreadLocked;
} KSPIN_LOCK, *PKSPIN_LOCK;

typedef struct _KLOCKED_LIST {
	KSPIN_LOCK Lock;
	PLIST_ENTRY List;
} KLOCKED_LIST, *PKLOCKED_LIST;

typedef struct _KPCR {
	ULONG32 ThreadQueueLength;
	PKTHREAD ThreadQueue;
	KSPIN_LOCK ThreadQueueLock;

	ULONG32 CpuIndex;
	ULONG32 AcpiId;

	GDTR Gdtr;
	IDTR Idtr;

	TSS TaskState;
	USHORT TaskStateDescriptor;

} KPCR, *PKPCR;

NTSYSAPI
VOID
KeInsertListEntry(
	__in LIST_ENTRY* ListHead,
	__in LIST_ENTRY* EntryToInsert
);

NTSYSAPI
VOID
KeRemoveListEntry(
	__in LIST_ENTRY* EntryToRemove
);

NTSYSAPI
VOID
KeInitializeListHead(
	__in LIST_ENTRY* EntryToInit
);

#define CONTAINING_RECORD(address, type, field) ((type*)((PCHAR)(address)-(ULONG64)(&((type *)0)->field)))

NTSYSAPI
VOID
KeAcquireSpinLock(
	__in PKSPIN_LOCK SpinLock
);

NTSYSAPI
VOID
KeReleaseSpinLock(
	__in PKSPIN_LOCK SpinLock
);

typedef VOID( *PKSTART_ROUTINE )( PVOID );

NTSYSAPI
NTSTATUS
KeCreateThread(
	__out PHANDLE ThreadHandle,
	__in HANDLE ProcessHandle,
	__in PKSTART_ROUTINE StartRoutine,
	__in PVOID StartContext,
	__in_opt ULONG32 KernelStackSize,
	__in_opt ULONG32 UserStackSize
);

NTSYSAPI
HANDLE
KeQueryCurrentThread(

);

NTSYSAPI
PKTHREAD
KiQueryCurrentThread(

);

NTSYSAPI
HANDLE
KeQueryCurrentProcess(

);

NTSYSAPI
PKPROCESS
KiQueryCurrentProcess(

);

NTSYSAPI
NTSTATUS
KeQueryLogicalProcessor(
	__in ULONG32 ProcessorIndex,
	__in PKPCR* Processor
);

NTSYSAPI
PKPCR
KeQueryCurrentProcessor(

);

NTSYSAPI
ULONG32
KeQueryProcessorCount(

);

NTSYSAPI
VOID
KeEnterCriticalRegion(

);

NTSYSAPI
VOID
KeLeaveCriticalRegion(

);

NTSYSAPI
NTSTATUS
KeDelayExecutionThread(
	__in ULONG64 Milliseconds
);

NTSYSAPI
NTSTATUS
KiSpinlockWaitThread(
	__in PKSPIN_LOCK SpinLock
);

NTSYSAPI
NTSTATUS
KeTerminateThread(
	__in HANDLE ThreadHandle
);

NTSYSAPI
DECLSPEC( noreturn )
VOID
KeExitThread(

);

NTSYSAPI
VOID
KeBugCheckEx(
	__in ULONG32 ExceptionCode,
	__in ULONG64 Arg1,
	__in ULONG64 Arg2,
	__in ULONG64 Arg3,
	__in ULONG64 Arg4
);

NTSYSAPI
VOID
KeProbeForRead(
	__in PVOID Address,
	__in ULONG Length
);

NTSYSAPI
VOID
KeProbeStringForRead(
	__in PUNICODE_STRING StringAddress
);

NTSYSAPI
VOID
KeProbeStringForWrite(
	__in PUNICODE_STRING StringAddress
);

NTSYSAPI
VOID
KeProbeForWrite(
	__in PVOID Address,
	__in ULONG Length
);

// eh.
#define EXCEPTION_DIV_BY_ZERO			( (ULONG32)0x80000001 )
#define EXCEPTION_BREAKPOINT            ( (ULONG32)0x80000002 )
#define EXCEPTION_ILLEGAL_INSTRUCTION	( (ULONG32)0x80000003 )
#define EXCEPTION_ACCESS_VIOLATION		( (ULONG32)0x80000004 )
#define EXCEPTION_DOUBLE_FAULT          ( (ULONG32)0x80000005 )

typedef CHAR KEXCEPTION_SEVERITY;

typedef enum _SEVERITY {
    ExceptionFatal,
    ExceptionNormal,
    ExceptionIgnore,
    ExceptionMaximum
} SEVERITY;

typedef CHAR KPROCESSOR_MODE;

typedef enum _MODE {
    KernelMode,
    UserMode,
    MaximumMode
} MODE;

typedef struct _CONTEXT {
	ULONG64 Rax;
	ULONG64 Rcx;
	ULONG64 Rdx;
	ULONG64 Rbx;
	ULONG64 Rsp;
	ULONG64 Rbp;
	ULONG64 Rsi;
	ULONG64 Rdi;
	ULONG64 R8;
	ULONG64 R9;
	ULONG64 R10;
	ULONG64 R11;
	ULONG64 R12;
	ULONG64 R13;
	ULONG64 R14;
	ULONG64 R15;
	ULONG64 Rip;

	ULONG64 EFlags;

	ULONG64 CodeSegment;
	ULONG64 DataSegment;
	ULONG64 StackSegment;
} CONTEXT, *PCONTEXT;

typedef struct _EXCEPTION_RECORD {
    PKTHREAD            ExceptionThread;
    PVAD                ExceptionVad;
    ULONG32             ExceptionCode;
    KPROCESSOR_MODE     ExceptionMode;
    KEXCEPTION_SEVERITY ExceptionSeverity;
    PVOID               ExceptionAddress;
    CONTEXT             ExceptionContext;

} EXCEPTION_RECORD, *PEXCEPTION_RECORD;

NTSYSAPI
DECLSPEC( noreturn )
VOID
KeRaiseException(
	__in ULONG32 ExceptionCode
);

typedef struct _SYSTEM_SERVICE {
	PVOID ServiceProcedure;
	ULONG ArgumentCount;
	ULONG Alignment;
} SYSTEM_SERVICE, *PSYSTEM_SERVICE;

typedef struct _KSYSTEM_SERVICE_DESCRIPTOR_TABLE {
	ULONG			ServiceCount;
	ULONG			Alignment;
	PSYSTEM_SERVICE ServiceTable;
} KSYSTEM_SERVICE_DESCRIPTOR_TABLE, *PKSYSTEM_SERVICE_DESCRIPTOR_TABLE;

NTSYSAPI
NTSTATUS
KeInstallServiceDescriptorTable(
	__in ULONG           ServiceTableIndex,
	__in ULONG			 ServiceCount,
	__in PSYSTEM_SERVICE ServiceTable
);

#define DECLARE_SYSTEM_SERVICE( x, y ) { x, y }
