


#pragma once

#ifndef NTSYSCALLAPI
#define NTSYSCALLAPI
#endif

NTSYSCALLAPI
NTSTATUS
NtCreateFile(
	__out PHANDLE            FileHandle,
	__out PIO_STATUS_BLOCK   IoStatusBlock,
	__in  ACCESS_MASK        DesiredAccess,
	__in  ULONG              Disposition,
	__in  POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSCALLAPI
NTSTATUS
NtReadFile(
	__in HANDLE FileHandle,
	__out PIO_STATUS_BLOCK IoStatusBlock,
	__in PVOID Buffer,
	__in ULONG64 Length,
	__in ULONG64 ByteOffset
);

NTSYSCALLAPI
NTSTATUS
NtWriteFile(
	__in HANDLE FileHandle,
	__out PIO_STATUS_BLOCK IoStatusBlock,
	__in PVOID Buffer,
	__in ULONG64 Length,
	__in ULONG64 ByteOffset
);

NTSYSCALLAPI
NTSTATUS
NtClose(
	__in HANDLE Handle
);

NTSYSCALLAPI
NTSTATUS
NtQueryDirectoryFile(
	__in HANDLE FileHandle,
	__out PIO_STATUS_BLOCK IoStatusBlock,
	__in PVOID FileInformation,
	__in ULONG Length,
	__in FILE_INFORMATION_CLASS FileInformationClass,
	__in_opt PUNICODE_STRING FileName
);

NTSYSCALLAPI
NTSTATUS
NtQueryInformationFile(
	__in HANDLE FileHandle,
	__out PIO_STATUS_BLOCK IoStatusBlock,
	__in PVOID FileInformation,
	__in ULONG Length,
	__in FILE_INFORMATION_CLASS FileInformationClass
);

NTSYSCALLAPI
NTSTATUS
NtSetInformationFile(
	__in HANDLE FileHandle,
	__out PIO_STATUS_BLOCK IoStatusBlock,
	__in PVOID FileInformation,
	__in ULONG Length,
	__in FILE_INFORMATION_CLASS FileInformationClass
);

NTSTATUS
NtDeviceIoControlFile(
	__in HANDLE FileHandle,
	__out PIO_STATUS_BLOCK IoStatusBlock,
	__in ULONG IoControlCode,
	__in PVOID InputBuffer,
	__in ULONG InputBufferLength,
	__in PVOID OutputBuffer,
	__in ULONG OutputBufferLength
);

NTSYSCALLAPI
NTSTATUS
NtDisplayString(
	__in PUNICODE_STRING String
);