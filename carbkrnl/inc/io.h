


#pragma once

typedef struct _OBJECT_SYMBOLIC_LINK {
    UNICODE_STRING LinkTarget;

} OBJECT_SYMBOLIC_LINK, *POBJECT_SYMBOLIC_LINK;

typedef struct _DRIVER_OBJECT *PDRIVER_OBJECT;
typedef struct _DEVICE_OBJECT *PDEVICE_OBJECT;
typedef struct _IRP *PIRP;

typedef NTSTATUS( *PKDRIVER_DISPATCH )(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP           Request
    );

typedef NTSTATUS( *PKDRIVER_LOAD )(
    _In_ PDRIVER_OBJECT DriverObject
    );

typedef VOID( *PKDRIVER_UNLOAD )(
    _In_ PDRIVER_OBJECT DriverObject
    );

#define IRP_MJ_CREATE                   0x00
#define IRP_MJ_CLOSE                    0x01
#define IRP_MJ_READ                     0x02
#define IRP_MJ_WRITE                    0x03
#define IRP_DIRECTORY_CONTROL           0x04
#define IRP_MJ_QUERY_INFORMATION_FILE   0x05
#define IRP_MJ_SET_INFORMATION_FILE     0x06
#define IRP_MJ_CONTROL                  0x07
#define IRP_MJ_CLEANUP                  0x08
#define IRP_MJ_MAX                      0x09

//
// DEV_FILE_SYSTEM
//

//
// DEV_DXGI (do not use)
//

typedef struct _DRIVER_OBJECT {
    UNICODE_STRING    DriverName;
    PKDRIVER_DISPATCH MajorFunction[ IRP_MJ_MAX ];
    PKDRIVER_LOAD     DriverLoad;
    PKDRIVER_UNLOAD   DriverUnload;
    PVOID             DriverVad; // dont touch this if ur a driver.
} DRIVER_OBJECT, *PDRIVER_OBJECT;

typedef struct _DEVICE_OBJECT {
    PDRIVER_OBJECT DriverObject;
    ULONG          DeviceCharacteristics;
    PVOID          DeviceExtension;
    ULONG64        DeviceExtensionSize;
    PDEVICE_OBJECT DeviceLink;
    ULONG32        StackLength;

} DEVICE_OBJECT, *PDEVICE_OBJECT;

#define DEV_INITIALIZING            0x00000001
#define DEV_EXCLUSIVE               0x00000002
#define DEV_BUFFERED_IO             0x00000004
#define DEV_DIRECT_IO               0x00000008
#define DEV_FORCE_SYNCHRONOUS_IO    0x00000010

//
// Interfaces supplied by the operating system
//
// these have not yet been developed.
//

#define DEV_FILE_SYSTEM             0x00000000
#define DEV_DXGI                    0x01000000

NTSYSAPI EXTERN POBJECT_TYPE IoSymbolicLinkObject;
NTSYSAPI EXTERN POBJECT_TYPE IoDriverObject;
NTSYSAPI EXTERN POBJECT_TYPE IoDeviceObject;
NTSYSAPI EXTERN POBJECT_TYPE IoFileObject;

typedef struct _VPB {
    ULONG32        Serial;
    ULONG32        Flags;
    UNICODE_STRING Label;
    PDEVICE_OBJECT DeviceObject;

} VPB, *PVPB;

typedef struct _IO_FILE_OBJECT {
    PVPB               Volume;
    PDEVICE_OBJECT     DeviceObject;
    UNICODE_STRING     FileName;
    ULONG64            FileLength;
    BOOLEAN            LockOperation;
    ULONG32            Disposition;
    ACCESS_MASK        ShareAccess;
    ACCESS_MASK        Access;
    PVOID              FsContext1;
    PVOID              FsContext2;
    NTSTATUS           FinalStatus;
    ULONG64            CurrentOffset;
    KEVENT             Lock; // impl NtLockFile, NtUnlockFile.
    PIO_FILE_OBJECT    Link; // REF COUNTED. not sure if this should stay.
    ULONG32            Flags;
    LIST_ENTRY         FileList;
    PMM_SECTION_OBJECT SectionObject;
} IO_FILE_OBJECT, *PIO_FILE_OBJECT;

//#define FO_

#define FILE_FLAG_DIRECTORY             (0x80000000L)
#define FILE_FLAG_ATTRIBUTE_HIDDEN      (0x00000001L)
#define FILE_FLAG_ATTRIBUTE_SYSTEM      (0x00000002L)
#define FILE_FLAG_ATTRIBUTE_READONLY    (0x00000004L)

typedef struct _IO_STACK_LOCATION {
    UCHAR MajorFunction;
    UCHAR MinorFunction;

    union {
        struct {
            ULONG32 Disposition;
            ULONG32 ShareAccess;
        } Create;

        struct {
            ULONG64 Length;
            ULONG64 Offset;
        } Read;

        struct {
            ULONG64 Length;
            ULONG64 Offset;
        } Write;

        struct {
            ULONG64 Length;
            FILE_INFORMATION_CLASS Info;
            BOOLEAN SingleMode;
            ULONG64 FileIndex;
        } DirectoryControl;

        struct {
            ULONG64 Length;
            FILE_INFORMATION_CLASS Info;
        } QueryFile;

        struct {
            ULONG64 Length;
            FILE_INFORMATION_CLASS Info;
        } SetFile;

        struct {
            ULONG64 OutputLength;
            ULONG64 InputLength;
            ULONG32 ControlCode;
        } Control;
    } Parameters;

} IO_STACK_LOCATION, *PIO_STACK_LOCATION;

typedef VOID( *PIO_COMPLETION_ROUTINE )(
    _In_ PIRP Request
    );

typedef struct _IRP {

    PDEVICE_OBJECT         DeviceObject;
    PIO_FILE_OBJECT        FileObject;
    KPROCESSOR_MODE        RequestorMode;
    PVOID                  SystemBuffer1;
    PVOID                  SystemBuffer2;
    IO_STATUS_BLOCK        IoStatus;
    KEVENT                 Event;

    PKPROCESS              Process;
    PKTHREAD               Thread;
    PIRP                   NextQueueIrp;

    PIO_COMPLETION_ROUTINE IoCompletion;

    struct {
        PKEVENT            Event;
        PIO_STATUS_BLOCK   IoStatus;
    } User;

    ULONG32                CurrentStack;
    IO_STACK_LOCATION      StackLocation[ 0 ];

} IRP, *PIRP;

NTSYSAPI
NTSTATUS
IoCreateSymbolicLink(
    _In_ PUNICODE_STRING LinkName,
    _In_ PUNICODE_STRING LinkTarget
);

NTSYSAPI
PIRP
IoAllocateIrp(
    _In_ ULONG32 StackCount
);

NTSYSAPI
VOID
IoFreeIrp(
    _In_ PIRP Request
);

FORCEINLINE
PIO_STACK_LOCATION
IoGetNextStackLocation(
    _In_ PIRP Request
)
{
    return &Request->StackLocation[ Request->CurrentStack + 1 ];
}

FORCEINLINE
PIO_STACK_LOCATION
IoGetCurrentStackLocation(
    _In_ PIRP Request
)
{
    return &Request->StackLocation[ Request->CurrentStack ];
}

FORCEINLINE
VOID
IoCopyCurrentIrpStackToNext(
    _In_ PIRP Request
)
{
    RtlCopyMemory(
        IoGetNextStackLocation( Request ),
        IoGetCurrentStackLocation( Request ),
        sizeof( IO_STACK_LOCATION ) );
}

FORCEINLINE
PDEVICE_OBJECT
IoGetNextDeviceOnStack(
    _In_ PIRP Request
)
{
    //
    // Potentially wrong, correct it.
    //
#if 0
    PDEVICE_OBJECT Device;
    ULONG64 Stack;

    Stack = Request->CurrentStack;

    Device = Request->FileObject->DeviceObject;
    while ( Stack-- ) {
        Device = Device->DeviceLink;
    }

    return Device->DeviceLink;
#endif// no refs
    Request;
    return NULL;

}

FORCEINLINE
VOID
IoCompleteRequest(
    _In_ PIRP Request
)
{
    KeSignalEvent( &Request->Event, TRUE );
    Request->IoCompletion( Request );
}

NTSYSAPI
NTSTATUS
IoCallDriver(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP           Request
);

NTSYSAPI
VOID
IoAttachDevice(
    _In_ PDEVICE_OBJECT High,
    _In_ PDEVICE_OBJECT Low
);

NTSYSAPI
NTSTATUS
IoCreateDevice(
    _In_  PDRIVER_OBJECT  DriverObject,
    _In_  ULONG64         DeviceExtensionSize,
    _In_  PUNICODE_STRING DeviceName,
    _In_  ULONG32         DeviceCharacteristics,
    _Out_ PDEVICE_OBJECT* DeviceObject
);

NTSYSAPI
PIRP
IoBuildSynchronousFsdRequest(
    _In_  PDEVICE_OBJECT   DeviceObject,
    _In_  UCHAR            MajorFunction,
    _In_  PVOID            Buffer,
    _In_  ULONG64          Length,
    _In_  ULONG64          Offset,
    _In_  PKEVENT          Event,
    _Out_ PIO_STATUS_BLOCK StatusBlock
);

NTSYSAPI
PIRP
IoBuildDeviceIoControlRequest(
    _In_  PDEVICE_OBJECT   DeviceObject,
    _In_  ULONG32          ControlCode,
    _In_  PVOID            InputBuffer,
    _In_  ULONG64          InputLength,
    _Out_ PVOID            OutputBuffer,
    _In_  ULONG64          OutputLength,
    _In_  PKEVENT          Event,
    _Out_ PIO_STATUS_BLOCK StatusBlock
);

NTSYSAPI
NTSTATUS
ZwCreateFile(
    _Out_ PHANDLE            FileHandle,
    _Out_ PIO_STATUS_BLOCK   IoStatusBlock,
    _In_  ACCESS_MASK        DesiredAccess,
    _In_  POBJECT_ATTRIBUTES ObjectAttributes,
    _In_  ULONG              Disposition,
    _In_  ULONG              ShareAccess,
    _In_  ULONG              CreateOptions
);

NTSTATUS
NtCreateFile(
    _Out_ PHANDLE            FileHandle,
    _Out_ PIO_STATUS_BLOCK   StatusBlock,
    _In_  ACCESS_MASK        DesiredAccess,
    _In_  POBJECT_ATTRIBUTES ObjectAttributes,
    _In_  ULONG              Disposition,
    _In_  ULONG              ShareAccess,
    _In_  ULONG              CreateOptions
);

NTSYSAPI
NTSTATUS
ZwReadFile(
    _In_    HANDLE           FileHandle,
    _In_    HANDLE           EventHandle,
    _Inout_ PIO_STATUS_BLOCK StatusBlock,
    _Out_   PVOID            Buffer,
    _In_    ULONG64          Length,
    _In_    ULONG64          Offset
);

NTSTATUS
NtReadFile(
    _In_    HANDLE           FileHandle,
    _In_    HANDLE           EventHandle,
    _Inout_ PIO_STATUS_BLOCK StatusBlock,
    _Out_   PVOID            Buffer,
    _In_    ULONG64          Length,
    _In_    ULONG64          Offset
);

NTSYSAPI
NTSTATUS
IoLoadDriver(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING DriverPath
);

NTSYSAPI
NTSTATUS
ZwDeviceIoControlFile(
    _In_    HANDLE           FileHandle,
    _In_    HANDLE           EventHandle,
    _Inout_ PIO_STATUS_BLOCK StatusBlock,
    _In_    ULONG32          ControlCode,
    _Out_   PVOID            InputBuffer,
    _In_    ULONG64          InputLength,
    _In_    PVOID            OutputBuffer,
    _In_    ULONG64          OutputLength
);

NTSTATUS
NtDeviceIoControlFile(
    _In_    HANDLE           FileHandle,
    _In_    HANDLE           EventHandle,
    _Inout_ PIO_STATUS_BLOCK StatusBlock,
    _In_    ULONG32          ControlCode,
    _Out_   PVOID            InputBuffer,
    _In_    ULONG64          InputLength,
    _In_    PVOID            OutputBuffer,
    _In_    ULONG64          OutputLength
);

EXTERN POBJECT_TYPE IoInterruptObject;

typedef struct _IO_INTERRUPT *PIO_INTERRUPT;

typedef struct _KINTERRUPT {

    //
    // Local structure declared when an interrupt 
    // fires, passed to handlers.
    //

    PIO_INTERRUPT   Interrupt;
    PKTRAP_FRAME    TrapFrame;
    KIRQL           PreviousIrql;
    KPROCESSOR_MODE PreviousMode;
    ULONG           Vector;

} KINTERRUPT, *PKINTERRUPT;

typedef BOOLEAN( *KSERVICE_ROUTINE )(
    _In_ PKINTERRUPT,
    _In_ PVOID
    );

NTSYSAPI
NTSTATUS
IoConnectInterrupt(
    _Out_ PIO_INTERRUPT*     InterruptObject,
    _In_  KSERVICE_ROUTINE   ServiceRoutine,
    _In_  PVOID              ServiceContext,
    _In_  ULONG              Vector,
    _In_  KIRQL              Irql,
    _In_  POBJECT_ATTRIBUTES Interrupt
);

NTSYSAPI
VOID
IoDisconnectInterrupt(
    _In_ PIO_INTERRUPT InterruptObject
);

//
// The above stuff is the async io queue
// this stuff is for interrupt handlers for
// making a statically allocated queue
//

typedef enum _IO_QUEUE_TYPE {
    //
    // Unused atm.
    //

    IoQueueSyncNotification,
    IoQueueAsyncNotification,
    IoQueueManual
} IO_QUEUE_TYPE, *PIO_QUEUE_TYPE;

typedef struct _IO_QUEUE {

    //
    // Potentially add some kind of association to 
    // IO_INTERRUPT objects and add a ref count (might not
    // be necessary if you associate with IO_INTERRUPT's).
    //
    // Potentially add a KEVENT too.
    //

    ULONG64    QueueBase;
    ULONG64    QueueLength;
    ULONG64    QueueHead;
    ULONG64    QueueTail;

    ULONG64    PacketLength;
    KSPIN_LOCK QueueLock;
    BOOLEAN    QueueFree;

} IO_QUEUE, *PIO_QUEUE;

NTSYSAPI
VOID
IoQueueCreate(
    _Out_ PIO_QUEUE Queue,
    _In_  ULONG64   QueueLength,
    _In_  ULONG64   PacketLength
);

NTSYSAPI
BOOLEAN
IoQueueEnqueue(
    _In_ PIO_QUEUE Queue,
    _In_ PVOID     Packet
);

NTSYSAPI
BOOLEAN
IoQueueDequeue(
    _In_  PIO_QUEUE Queue,
    _Out_ PVOID     Packet
);

NTSYSAPI
VOID
IoQueueFree(
    _In_ PIO_QUEUE Queue
);

NTSYSAPI
VOID
IoAcquireInterruptSafeLock(
    _In_ PKSPIN_LOCK SpinLock
);

NTSYSAPI
VOID
IoReleaseInterruptSafeLock(
    _In_ PKSPIN_LOCK SpinLock
);

NTSYSAPI
NTSTATUS
ZwQueryInformationFile(
    _In_  HANDLE                 FileHandle,
    _Out_ PIO_STATUS_BLOCK       StatusBlock,
    _Out_ PVOID                  FileInformation,
    _In_  ULONG64                Length,
    _In_  FILE_INFORMATION_CLASS FileInformationClass
);

NTSTATUS
NtQueryInformationFile(
    _In_  HANDLE                 FileHandle,
    _Out_ PIO_STATUS_BLOCK       StatusBlock,
    _Out_ PVOID                  FileInformation,
    _In_  ULONG64                Length,
    _In_  FILE_INFORMATION_CLASS FileInformationClass
);

NTSYSAPI
NTSTATUS
ZwQueryDirectoryFile(
    _In_  HANDLE                 FileHandle,
    _Out_ PIO_STATUS_BLOCK       StatusBlock,
    _Out_ PVOID                  FileInformation,
    _In_  ULONG64                Length,
    _In_  FILE_INFORMATION_CLASS FileInformationClass,
    _In_  PUNICODE_STRING        FileName,
    _In_  ULONG64                FileIndex,
    _In_  BOOLEAN                SingleMode
);

NTSTATUS
NtQueryDirectoryFile(
    _In_  HANDLE                 FileHandle,
    _Out_ PIO_STATUS_BLOCK       StatusBlock,
    _Out_ PVOID                  FileInformation,
    _In_  ULONG64                Length,
    _In_  FILE_INFORMATION_CLASS FileInformationClass,
    _In_  PUNICODE_STRING        FileName,
    _In_  ULONG64                FileIndex,
    _In_  BOOLEAN                SingleMode
);
