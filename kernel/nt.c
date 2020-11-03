


#include <carbsup.h>
#include "nt.h"

NTSTATUS
NtCreateFile(
	__out PHANDLE FileHandle,
	__out PIO_STATUS_BLOCK IoStatusBlock,
	__in ACCESS_MASK DesiredAccess,
	__in ULONG Disposition,
	__in POBJECT_ATTRIBUTES ObjectAttributes
)
{
	FileHandle;
	IoStatusBlock;
	DesiredAccess;
	Disposition;
	ObjectAttributes;

	return STATUS_SUCCESS;
}

NTSTATUS
NtReadFile(
	__in HANDLE FileHandle,
	__out PIO_STATUS_BLOCK IoStatusBlock,
	__in PVOID Buffer,
	__in ULONG64 Length,
	__in ULONG64 ByteOffset
)
{
	FileHandle;
	IoStatusBlock;
	Buffer;
	Length;
	ByteOffset;

	return STATUS_SUCCESS;
}

NTSTATUS
NtWriteFile(
	__in HANDLE FileHandle,
	__out PIO_STATUS_BLOCK IoStatusBlock,
	__in PVOID Buffer,
	__in ULONG64 Length,
	__in ULONG64 ByteOffset
)
{
	FileHandle;
	IoStatusBlock;
	Buffer;
	Length;
	ByteOffset;

	return STATUS_SUCCESS;
}

NTSTATUS
NtClose(
	__in HANDLE Handle
)
{
	Handle;

	return STATUS_SUCCESS;
}

NTSTATUS
NtQueryDirectoryFile(
	__in HANDLE FileHandle,
	__out PIO_STATUS_BLOCK IoStatusBlock,
	__in PVOID FileInformation,
	__in ULONG Length,
	__in FILE_INFORMATION_CLASS FileInformationClass,
	__in_opt PUNICODE_STRING FileName
)
{
	FileHandle;
	IoStatusBlock;
	FileInformation;
	Length;
	FileInformationClass;
	FileName;

	return STATUS_SUCCESS;
}

NTSTATUS
NtQueryInformationFile(
	__in HANDLE FileHandle,
	__out PIO_STATUS_BLOCK IoStatusBlock,
	__in PVOID FileInformation,
	__in ULONG Length,
	__in FILE_INFORMATION_CLASS FileInformationClass
)
{
	FileHandle;
	IoStatusBlock;
	FileInformation;
	Length;
	FileInformationClass;

	return STATUS_SUCCESS;
}

NTSTATUS
NtSetInformationFile(
	__in HANDLE FileHandle,
	__out PIO_STATUS_BLOCK IoStatusBlock,
	__in PVOID FileInformation,
	__in ULONG Length,
	__in FILE_INFORMATION_CLASS FileInformationClass
)
{
	FileHandle;
	IoStatusBlock;
	FileInformation;
	Length;
	FileInformationClass;

	return STATUS_SUCCESS;
}

NTSTATUS
NtDeviceIoControlFile(
	__in HANDLE FileHandle,
	__out PIO_STATUS_BLOCK IoStatusBlock,
	__in ULONG IoControlCode,
	__in PVOID InputBuffer,
	__in ULONG InputBufferLength,
	__in PVOID OutputBuffer,
	__in ULONG OutputBufferLength
)
{
	FileHandle;
	IoStatusBlock;
	IoControlCode;
	InputBuffer;
	InputBufferLength;
	OutputBuffer;
	OutputBufferLength;

	return STATUS_SUCCESS;
}

NTSTATUS
NtDisplayString(
	__in PUNICODE_STRING String
)
{

	//String->Buffer

	if ( !MmIsAddressRangeValid( String, sizeof( UNICODE_STRING ) ) ||
		 !NT_SUCCESS( RtlUnicodeStringValidate( String ) ) ||
		 !MmIsAddressRangeValid( String->Buffer, String->Size ) ) {

		KeRaiseException( STATUS_ACCESS_VIOLATION );
	}

	printf( "%w", String->Buffer );

	return STATUS_SUCCESS;
}