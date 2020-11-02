/*++

Module ObjectName:

	ke.h

Abstract:

	1

--*/

#pragma once

#include "ldrpsup.h"
#include "hal.h"

VOID
KiBugCheckTrap(
	__inout PKTRAP_FRAME TrapFrame,
	__in PKPCR Processor
);

NTSTATUS
KiCreateKpcr(
	__in ULONG32 AcpiId,
	__out PKPCR* Kpcr
);

#include "ki_struct.h"

ULONG32
KiGetUniqueIdentifier(

);

NTSTATUS
KeQueryLogicalProcessor(
	__in ULONG32 ProcessorIndex,
	__in PKPCR* Kpcr
);

VOID
KiThreadDispatcher(
	__inout PKTRAP_FRAME TrapFrame,
	__in PKPCR Processor
);

NTSTATUS
KiCreateThread(
	__out PKTHREAD* NewThreadHandle,
	__in PKPROCESS ThreadProcess,
	__in PKSTART_ROUTINE StartRoutine,
	__in PVOID StartContext,
	__in_opt ULONG32 KernelStackSize,
	__in_opt ULONG32 UserStackSize
);

NTSTATUS
KiCreateProcess(
	__out PKPROCESS* NewProcessHandle,
	__in PKMODULE AssociatedModule,
	__in PUNICODE_STRING ProcessName
);

VOID
KiInitializeDispatcher(
	__in PKPROCESS KernelProcess
);

VOID
KiIdleThread(

);

VOID
KiStartThread(
	__in PKTHREAD Thread
);

NTSTATUS
KeCreateThread(
	__out PHANDLE ThreadHandle,
	__in HANDLE ProcessHandle,
	__in PKSTART_ROUTINE StartRoutine,
	__in PVOID StartContext,
	__in_opt ULONG32 KernelStackSize,
	__in_opt ULONG32 UserStackSize
);

PKPCR
KeQueryCurrentProcessor(

);

PKPROCESS
KiQueryCurrentProcess(

);

HANDLE
KeQueryCurrentProcess(

);

PKTHREAD
KiQueryCurrentThread(

);

HANDLE
KeQueryCurrentThread(

);

EXTERN ULONG32 KiLogicalProcessorsInstalled;

NTSTATUS
KeProbeForRead(
	__in PVOID Address,
	__in ULONG Length
);

NTSTATUS
KeProbeForWrite(
	__in PVOID Address,
	__in ULONG Length
);

VOID
KiBspBootBugcheck(
	__in ULONG32 ExceptionCode,
	__in ULONG64 Arg1,
	__in ULONG64 Arg2,
	__in ULONG64 Arg3,
	__in ULONG64 Arg4
);

ULONG32
KiAcquireLowestWorkProcessor(

);

VOID
KiInitializeSyscalls(

);
