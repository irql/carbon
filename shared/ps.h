


#pragma once

NTSYSAPI
NTSTATUS
PsCreateUserProcess(
	__out PHANDLE         ProcessHandle,
	__in  PUNICODE_STRING FileName
);

NTSYSAPI
NTSTATUS
PsCreateUserThread(
	__out    PHANDLE         ThreadHandle,
	__in     HANDLE          ProcessHandle,
	__in     PKSTART_ROUTINE ThreadStart,
	__in     PVOID           ThreadContext,
	__in     ULONG32         ThreadFlags,
	__in_opt ULONG32         UserStackSize,
	__in_opt ULONG32         KernelStackSize
);