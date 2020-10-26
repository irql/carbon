/*++

Module ObjectName:

	io.h

Abstract:

	I/O manager.

--*/

#pragma once

typedef ULONG32 ACCESS_MASK;
typedef ACCESS_MASK *PACCESS_MASK;

#define FILE_CREATED					(0x00000001L)
#define FILE_OPENED						(0x00000002L)
#define FILE_OVERWRITTEN				(0x00000004L)
#define FILE_EXISTS						(0x00000008L)
#define FILE_DOES_NOT_EXIST				(0x00000010L)

#define GENERIC_WRITE					(0x20000000L)
#define GENERIC_READ					(0x10000000L)

#define FILE_FLAG_DIRECTORY				(0x80000000L)
#define FILE_FLAG_ATTRIBUTE_HIDDEN		(0x00000001L)
#define FILE_FLAG_ATTRIBUTE_SYSTEM		(0x00000002L)
#define FILE_FLAG_ATTRIBUTE_READONLY	(0x00000004L)

#define IRP_MJ_CREATE					0x00
#define IRP_MJ_CLOSE					0x01
#define IRP_MJ_READ						0x02
#define IRP_MJ_WRITE					0x03
#define IRP_MJ_QUERY_DIRECTORY_FILE		0x04
#define IRP_MJ_QUERY_INFORMATION_FILE	0x05
#define IRP_MJ_SET_INFORMATION_FILE		0x06
#define IRP_MJ_CONTROL					0x07
#define IRP_MJ_MAX						IRP_MJ_CONTROL+1

typedef CHAR KPROCESSOR_MODE;

typedef enum _MODE {
	KernelMode,
	UserMode,
	MaximumMode
} MODE;

typedef enum _FILE_INFORMATION_CLASS {
	FileDirectoryInformation = 1,
	FileBasicInformation,

} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

typedef struct _FILE_BASIC_INFORMATION {
	ULONG64 FileSize;
	ULONG64 FileSizeOnDisk;

} FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;

typedef struct _IO_STATUS_BLOCK {
	NTSTATUS Status;
	ULONG64 Information;

} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef struct _DRIVER_OBJECT *PDRIVER_OBJECT;
typedef struct _DEVICE_OBJECT *PDEVICE_OBJECT;
typedef struct _IRP *PIRP;

typedef NTSTATUS KDRIVER_DISPATCH(
	__in PDEVICE_OBJECT,
	__in PIRP
);
typedef KDRIVER_DISPATCH *PKDRIVER_DISPATCH;

typedef VOID KDRIVER_UNLOAD(
	__in PDRIVER_OBJECT
);
typedef KDRIVER_UNLOAD *PKDRIVER_UNLOAD;

typedef VOID KDRIVER_LOAD(
	__in PDRIVER_OBJECT
);
typedef KDRIVER_LOAD *PKDRIVER_LOAD;

typedef struct _DRIVER_OBJECT {

	//
	//	DriverObject's should just have driver specific 
	//	information, DeviceObject's should contain anything
	//	to do with I/O.
	//

	PKMODULE DriverModule;
	UNICODE_STRING DriverName;

	PKDRIVER_LOAD DriverLoad;
	PKDRIVER_UNLOAD DriverUnload;


} DRIVER_OBJECT, *PDRIVER_OBJECT;

typedef struct _DEVICE_OBJECT {
	PDRIVER_OBJECT DriverObject;

	PKDRIVER_DISPATCH MajorFunction[ IRP_MJ_MAX ];
} DEVICE_OBJECT, *PDEVICE_OBJECT;

#if 0
typedef struct _VPB {
	ULONG32 Flags;
	ULONG32 SerialNumber;
	ULONG32 ReferenceCount;
	USHORT VolumeLabelLength;
	WCHAR VolumeLabel[ 32 ];
	PDEVICE_OBJECT DeviceObject;
} VPB, *PVPB;
#endif

typedef struct _FILE_OBJECT {
	//PVPB Volume;

	ULONG32 DiskId;
	ULONG32 PartitionId;

	ULONG32 Access;
	ULONG32 Disposition;
	ULONG32 Flags;

	NTSTATUS FinalStatus;

	PVOID FileMemory;
	ULONG64 FileMemorySize;
	ULONG64 FileSize;

	//KSPIN_LOCK FileLock; will only be touched by the disk driver right?

	PUNICODE_STRING FileName;//complete path.
	PUNICODE_STRING FsFileName;//used by drivers for example in the disk driver it places the path after the partition here.

	//
	//	The initial FILE_OBJECT should point to the containing directory.
	//	This field should point to this FILE_OBJECT (if a directory was specified for
	//	creation) or the file at the end of the path.
	//

	struct _FILE_OBJECT* DirectoryFile;
} FILE_OBJECT, *PFILE_OBJECT;

typedef struct _IO_STACK_LOCATION {
	UCHAR MajorFunction;
	UCHAR MinorFunction;

	union {

		struct {
			ACCESS_MASK Access;
			ULONG32 Disposition;

		} Create;

		struct {
			ULONG64 Length;
			ULONG64 ByteOffset;

		} Read;

		struct {
			ULONG64 Length;
			ULONG64 ByteOffset;

		} Write;

		struct {
			ULONG32 Length;
			PUNICODE_STRING FileName;
			FILE_INFORMATION_CLASS FileInformationClass;

		} QueryDirectory;

		//SetDirectory?

		struct {
			ULONG32 Length;
			FILE_INFORMATION_CLASS FileInformationClass;

		} QueryFile;

		struct {
			ULONG32 Length;
			FILE_INFORMATION_CLASS FileInformationClass;

		} SetFile;

		struct {
			ULONG32 OutputBufferLength;
			ULONG32 InputBufferLength;
			ULONG32 ControlCode;
		} Control;


	} Parameters;

} IO_STACK_LOCATION, *PIO_STACK_LOCATION;

typedef struct _IRP {

	KPROCESSOR_MODE RequestorMode;

	PVOID SystemBuffer;
	PVOID UserBuffer;

	IO_STATUS_BLOCK UserIosb;
	PKPROCESS IssuingProcess;

	PKTHREAD Thread;

	PIO_STACK_LOCATION StackLocation;

	PFILE_OBJECT FileObject;

} IRP, *PIRP;


NTSYSAPI
NTSTATUS
IoCallDevice(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp
);

NTSYSAPI
PIRP
IoBuildDeviceIoControlRequest(
	__in ULONG ControlCode,
	__in PVOID InputBuffer,
	__in ULONG InputBufferLength,
	__in PVOID OutputBuffer,
	__in ULONG OutputBufferLength
);

NTSYSAPI
NTSTATUS
IoCreateDevice(
	__in PDRIVER_OBJECT DriverObject,
	__in PUNICODE_STRING DeviceName,
	__out PDEVICE_OBJECT* DeviceObject
);

NTSYSAPI
NTSTATUS
ZwCreateFile(
	__out PHANDLE FileHandle,
	__out PIO_STATUS_BLOCK IoStatusBlock,
	__in ACCESS_MASK DesiredAccess,
	__in ULONG Disposition,
	__in POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
ZwReadFile(
	__in HANDLE FileHandle,
	__out PIO_STATUS_BLOCK IoStatusBlock,
	__in PVOID Buffer,
	__in ULONG64 Length,
	__in ULONG64 ByteOffset
);

NTSYSAPI
NTSTATUS
ZwWriteFile(
	__in HANDLE FileHandle,
	__out PIO_STATUS_BLOCK IoStatusBlock,
	__in PVOID Buffer,
	__in ULONG64 Length,
	__in ULONG64 ByteOffset
);

NTSYSAPI
NTSTATUS
ZwClose(
	__in HANDLE Handle
);

NTSYSAPI
NTSTATUS
ZwQueryDirectoryFile(
	__in HANDLE FileHandle,
	__out PIO_STATUS_BLOCK IoStatusBlock,
	__in PVOID FileInformation,
	__in ULONG Length,
	__in FILE_INFORMATION_CLASS FileInformationClass,
	__in_opt PUNICODE_STRING FileName
);

NTSYSAPI
NTSTATUS
ZwQueryInformationFile(
	__in HANDLE FileHandle,
	__out PIO_STATUS_BLOCK IoStatusBlock,
	__in PVOID FileInformation,
	__in ULONG Length,
	__in FILE_INFORMATION_CLASS FileInformationClass
);

NTSYSAPI
NTSTATUS
ZwSetInformationFile(
	__in HANDLE FileHandle,
	__out PIO_STATUS_BLOCK IoStatusBlock,
	__in PVOID FileInformation,
	__in ULONG Length,
	__in FILE_INFORMATION_CLASS FileInformationClass
);

NTSYSAPI
NTSTATUS
ZwDeviceIoControlFile(
	__in HANDLE FileHandle,
	__out PIO_STATUS_BLOCK IoStatusBlock,
	__in ULONG IoControlCode,
	__in PVOID InputBuffer,
	__in ULONG InputBufferLength,
	__in PVOID OutputBuffer,
	__in ULONG OutputBufferLength
);

NTSYSAPI
NTSTATUS
IoLoadDriver(
	__in PUNICODE_STRING FilePath
);