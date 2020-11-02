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
	__in PKPCR* Kpcr
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


