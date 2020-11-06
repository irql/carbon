


#pragma once

#ifndef NTSYSCALLAPI
#define NTSYSCALLAPI
#endif

NTSYSCALLAPI
NTSTATUS
NtGdiDisplayString(
	__in PUNICODE_STRING String
);

NTSYSCALLAPI
NTSTATUS
NtGdiCreateConsole(
	__out PHANDLE		  ConsoleHandle,
	__in  PUNICODE_STRING Name,
	__in  ULONG32		  Flags,
	__in  ULONG32		  x,
	__in  ULONG32		  y
);

NTSYSCALLAPI
NTSTATUS
NtGdiReadConsole(
	__in HANDLE  ConsoleHandle,
	__in PWCHAR  Buffer,
	__in ULONG32 Length
);

NTSYSCALLAPI
NTSTATUS
NtGdiWriteConsole(
	__in HANDLE  ConsoleHandle,
	__in PWCHAR  Buffer,
	__in ULONG32 Length
);