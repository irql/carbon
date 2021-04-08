


#include <carbsup.h>
#include "iop.h"

//
// These are lists of files that are opened,
// this saves a lot of memory and should be
// pretty poggers.
//

ULONG64     IopFileCharge = 0;
KSPIN_LOCK  IopFileListLock = { 0 };
PLIST_ENTRY IopFileList = NULL;

VOID
IopInsertFileObject(
    _In_ PIO_FILE_OBJECT FileObject
)
{
    KIRQL PreviousIrql;
    KeAcquireSpinLock( &IopFileListLock, &PreviousIrql );
    if ( IopFileList == NULL ) {
        KeInitializeHeadList( &FileObject->FileList );
        IopFileList = &FileObject->FileList;
    }
    else {
        KeInsertHeadList( IopFileList, &FileObject->FileList );
    }
    IopFileCharge++;
    KeReleaseSpinLock( &IopFileListLock, PreviousIrql );
}

VOID
IopRemoveFileObject(
    _In_ PIO_FILE_OBJECT FileObject
)
{
    KIRQL PreviousIrql;
    KeAcquireSpinLock( &IopFileListLock, &PreviousIrql );
    IopFileCharge--;
    if ( IopFileCharge == 0 ) {
        IopFileList = NULL;
    }
    else {
        KeRemoveList( &FileObject->FileList );
    }
    KeReleaseSpinLock( &IopFileListLock, PreviousIrql );
}

NTSTATUS
IopFindCachedFileObject(
    _Out_ PIO_FILE_OBJECT* FileObject,
    _In_  PDEVICE_OBJECT   DeviceObject,
    _In_  PUNICODE_STRING  FileName,
    _In_  ULONG32          DesiredAccess,
    _In_  ULONG32          ShareAccess
)
{
    KIRQL PreviousIrql;
    PLIST_ENTRY Flink;
    PIO_FILE_OBJECT CurrentFileObject;

    //
    // TODO: Optimize this procedure.
    //

    if ( IopFileCharge == 0 ) {

        return STATUS_NOT_FOUND;
    }

    DesiredAccess &= GENERIC_READ | GENERIC_WRITE;

    KeAcquireSpinLock( &IopFileListLock, &PreviousIrql );

    Flink = IopFileList;
    do {
        CurrentFileObject = CONTAINING_RECORD( Flink, IO_FILE_OBJECT, FileList );
        Flink = Flink->Flink;

        if ( DeviceObject != CurrentFileObject->DeviceObject ) {

            continue;
        }

        if ( ( FileName->Buffer == NULL && CurrentFileObject->FileName.Buffer != NULL ) ||
            ( FileName->Buffer != NULL && CurrentFileObject->FileName.Buffer == NULL ) ) {

            continue;
        }

        if ( ( CurrentFileObject->FileName.Buffer == NULL && FileName->Buffer == NULL ) ||
             RtlCompareUnicodeString( FileName, &CurrentFileObject->FileName, TRUE ) == 0 ) {

            if ( ( CurrentFileObject->ShareAccess & DesiredAccess ) == DesiredAccess &&
                 CurrentFileObject->ShareAccess == ShareAccess ) {

                ObReferenceObject( CurrentFileObject );
                *FileObject = CurrentFileObject;
                KeReleaseSpinLock( &IopFileListLock, PreviousIrql );
                return STATUS_SUCCESS;
            }
            else {
                KeReleaseSpinLock( &IopFileListLock, PreviousIrql );
                return STATUS_ACCESS_DENIED;
            }
        }

    } while ( Flink != IopFileList );

    KeReleaseSpinLock( &IopFileListLock, PreviousIrql );
    return STATUS_NOT_FOUND;
}

NTSTATUS
IoFindCachedFileObject(
    _Out_ PIO_FILE_OBJECT* FileObject,
    _In_  PDEVICE_OBJECT   DeviceObject,
    _In_  PWSTR            FileName
)
{
    KIRQL PreviousIrql;
    PLIST_ENTRY Flink;
    PIO_FILE_OBJECT CurrentFileObject;

    //
    // File system drivers might want to call this to update
    // FsContext1 structures or other information when a 
    // file is changed. 
    //

    KeAcquireSpinLock( &IopFileListLock, &PreviousIrql );

    Flink = IopFileList;
    do {
        CurrentFileObject = CONTAINING_RECORD( Flink, IO_FILE_OBJECT, FileList );
        Flink = Flink->Flink;

        if ( DeviceObject != CurrentFileObject->DeviceObject ) {

            continue;
        }

        if ( ( FileName == NULL && CurrentFileObject->FileName.Buffer != NULL ) ||
            ( FileName != NULL && CurrentFileObject->FileName.Buffer == NULL ) ) {

            continue;
        }

        if ( ( CurrentFileObject->FileName.Buffer == NULL && FileName == NULL ) ||
             RtlCompareString( CurrentFileObject->FileName.Buffer, FileName, TRUE ) ) {

            ObReferenceObject( CurrentFileObject );
            *FileObject = CurrentFileObject;
            KeReleaseSpinLock( &IopFileListLock, PreviousIrql );
            return STATUS_SUCCESS;
        }

    } while ( Flink != IopFileList );

    KeReleaseSpinLock( &IopFileListLock, PreviousIrql );
    return STATUS_NOT_FOUND;
}

VOID
IoCompletionRoutine(
    _In_ PIRP Request
)
{
    //
    // If a driver hooks IoCompletion they should call this afterwards,
    // or repeat it's behaviour.
    //

    if ( Request->User.IoStatus != NULL ) {

        RtlCopyMemory( Request->User.IoStatus, &Request->IoStatus, sizeof( IO_STATUS_BLOCK ) );
    }

    if ( Request->User.Event != NULL ) {

        KeSignalEvent( Request->User.Event, TRUE );
    }

}

VOID
IopCleanupFile(
    _In_ PIO_FILE_OBJECT FileObject
)
{
    PIRP Request;

    Request = IoAllocateIrp( FileObject->DeviceObject->StackLength );
    Request->Process = PsGetCurrentProcess( );
    Request->Thread = PsGetCurrentThread( );
    Request->RequestorMode = KernelMode;
    Request->DeviceObject = FileObject->DeviceObject;
    Request->FileObject = FileObject;

    Request->IoCompletion = IoCompletionRoutine;
    Request->User.IoStatus = NULL;
    Request->User.Event = NULL;

    Request->StackLocation[ 0 ].MajorFunction = IRP_MJ_CLEANUP;
    Request->StackLocation[ 0 ].MinorFunction = 0;

    IoCallDriver( FileObject->DeviceObject, Request );
    IoFreeIrp( Request );
    MmFreePoolWithTag( FileObject->FileName.Buffer, IO_TAG );
    IopRemoveFileObject( FileObject );
    ObDereferenceObject( FileObject->DeviceObject );
}

PIRP
IoBuildSynchronousFsdRequest(
    _In_      PDEVICE_OBJECT   DeviceObject,
    _In_      UCHAR            MajorFunction,
    _In_      PVOID            Buffer,
    _In_      ULONG64          Length,
    _In_      ULONG64          Offset,
    _Out_opt_ PKEVENT          Event,
    _Out_opt_ PIO_STATUS_BLOCK StatusBlock
)
{
    //
    // WARNING: No file object is associated with any irp
    // created by this procedure, this could cause some drivers to crash.
    //
    // Function is designed for usage with only IRP_MJ_READ and IRP_MJ_WRITE,
    // this call will usually be proceeded with an IoCallDriver with the Request
    // parameter as this return value.
    //

    PIRP Request;

    Request = IoAllocateIrp( DeviceObject->StackLength );
    Request->Process = PsGetCurrentProcess( );
    Request->Thread = PsGetCurrentThread( );
    Request->RequestorMode = PsGetPreviousMode( Request->Thread );
    Request->DeviceObject = DeviceObject;

    ObReferenceObject( Request->Process );
    ObReferenceObject( Request->Thread );

    Request->SystemBuffer1 = Buffer;

    Request->StackLocation[ 0 ].MajorFunction = MajorFunction;
    Request->StackLocation[ 0 ].MinorFunction = 0;
    Request->StackLocation[ 0 ].Parameters.Read.Length = Length;
    Request->StackLocation[ 0 ].Parameters.Read.Offset = Offset;

    Request->FileObject = NULL;

    Request->IoCompletion = IoCompletionRoutine;
    Request->User.Event = Event;
    Request->User.IoStatus = StatusBlock;

    return Request;
}

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
)
{
    //
    // Same warning applies ^
    //

    PIRP Request;

    Request = IoAllocateIrp( DeviceObject->StackLength );
    Request->Process = PsGetCurrentProcess( );
    Request->Thread = PsGetCurrentThread( );
    Request->RequestorMode = PsGetPreviousMode( Request->Thread );
    Request->DeviceObject = DeviceObject;

    ObReferenceObject( Request->Process );
    ObReferenceObject( Request->Thread );

    Request->SystemBuffer1 = InputBuffer;
    Request->SystemBuffer1 = OutputBuffer;

    Request->StackLocation[ 0 ].MajorFunction = IRP_MJ_CONTROL;
    Request->StackLocation[ 0 ].MinorFunction = 0;
    Request->StackLocation[ 0 ].Parameters.Control.ControlCode = ControlCode;
    Request->StackLocation[ 0 ].Parameters.Control.InputLength = InputLength;
    Request->StackLocation[ 0 ].Parameters.Control.OutputLength = OutputLength;

    Request->FileObject = NULL;

    Request->IoCompletion = IoCompletionRoutine;
    Request->User.Event = Event;
    Request->User.IoStatus = StatusBlock;

    return Request;
}

NTSTATUS
ZwCreateFile(
    _Out_ PHANDLE            FileHandle,
    _Out_ PIO_STATUS_BLOCK   StatusBlock,
    _In_  ACCESS_MASK        DesiredAccess,
    _In_  POBJECT_ATTRIBUTES ObjectAttributes,
    _In_  ULONG              Disposition,
    _In_  ULONG              ShareAccess,
    _In_  ULONG              CreateOptions
)
{
    FileHandle;
    StatusBlock;
    DesiredAccess;
    ObjectAttributes;
    Disposition;
    ShareAccess;
    CreateOptions;

    STATIC OBJECT_ATTRIBUTES FileAttributes = { 0 };

    NTSTATUS ntStatus;
    PIO_FILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    PIRP Request;

    ntStatus = ObReferenceObjectByName( &DeviceObject,
                                        &ObjectAttributes->ObjectName,
                                        IoDeviceObject );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    ntStatus = IopFindCachedFileObject( &FileObject,
                                        DeviceObject,
                                        &ObjectAttributes->RootDirectory,
                                        DesiredAccess,
                                        ShareAccess );
    if ( ntStatus != STATUS_NOT_FOUND &&
         !NT_SUCCESS( ntStatus ) ) {

        ObDereferenceObject( DeviceObject );
        return ntStatus;
    }

    if ( ntStatus == STATUS_NOT_FOUND ) {
        ntStatus = ObCreateObject( &FileObject,
                                   IoFileObject,
                                   &FileAttributes,
                                   sizeof( IO_FILE_OBJECT ) );
        if ( !NT_SUCCESS( ntStatus ) ) {

            ObDereferenceObject( DeviceObject );
            return ntStatus;
        }

        RtlCopyMemory( &FileObject->FileName, &ObjectAttributes->RootDirectory, sizeof( UNICODE_STRING ) );
        FileObject->FileName.Buffer = MmAllocatePoolWithTag( NonPagedPoolZeroed, FileObject->FileName.MaximumLength, IO_TAG );
        RtlCopyMemory( FileObject->FileName.Buffer, ObjectAttributes->RootDirectory.Buffer, FileObject->FileName.Length );

        FileObject->Access = DesiredAccess;
        FileObject->ShareAccess = ShareAccess;
        FileObject->DeviceObject = DeviceObject;

        IopInsertFileObject( FileObject );
    }

    ntStatus = ObOpenObjectFromPointer( FileHandle,
                                        FileObject,
                                        DesiredAccess,
                                        ObjectAttributes->Attributes,
                                        KernelMode );
    if ( !NT_SUCCESS( ntStatus ) ) {

        //
        // TODO: This ObDeref will cause an IRP_MJ_CLEANUP to be sent, 
        // without any IRP_MJ_CREATE.
        //

        ObDereferenceObject( FileObject );
        ObDereferenceObject( DeviceObject );
        return ntStatus;
    }

    Request = IoAllocateIrp( FileObject->DeviceObject->StackLength );
    Request->Process = PsGetCurrentProcess( );
    Request->Thread = PsGetCurrentThread( );
    Request->RequestorMode = PsGetPreviousMode( Request->Thread );
    Request->DeviceObject = FileObject->DeviceObject;
    Request->FileObject = FileObject;

    ObReferenceObject( Request->Process );
    ObReferenceObject( Request->Thread );

    //
    // Irp_mj_create's are always synchronous (maybe change this in the future)
    //

    Request->IoCompletion = IoCompletionRoutine;
    Request->User.IoStatus = StatusBlock;
    Request->User.Event = NULL;

    Request->StackLocation[ 0 ].MajorFunction = IRP_MJ_CREATE;
    Request->StackLocation[ 0 ].MinorFunction = 0;
    Request->StackLocation[ 0 ].Parameters.Create.Disposition = Disposition;
    Request->StackLocation[ 0 ].Parameters.Create.ShareAccess = ShareAccess;

    ntStatus = IoCallDriver( FileObject->DeviceObject, Request );

    ObDereferenceObject( Request->Process );
    ObDereferenceObject( Request->Thread );
#if 0
    if ( !NT_SUCCESS( Request->IoStatus.Status ) ) {

        IoFreeIrp( Request );
        ObDereferenceObject( FileObject );
        ZwClose( *FileHandle );
        return STATUS_UNSUCCESSFUL;
    }
#endif

    IoFreeIrp( Request );
    ObDereferenceObject( FileObject );
    return ntStatus;
}

NTSTATUS
NtCreateFile(
    _Out_ PHANDLE            FileHandle,
    _Out_ PIO_STATUS_BLOCK   StatusBlock,
    _In_  ACCESS_MASK        DesiredAccess,
    _In_  POBJECT_ATTRIBUTES ObjectAttributes,
    _In_  ULONG              Disposition,
    _In_  ULONG              ShareAccess,
    _In_  ULONG              CreateOptions
)
{
    //
    // all Nt* functions need fixing up, this is not a proper
    // way of handling exceptions.
    //

    __try {
        return ZwCreateFile( FileHandle,
                             StatusBlock,
                             DesiredAccess,
                             ObjectAttributes,
                             Disposition,
                             ShareAccess,
                             CreateOptions );

    }
    __except ( EXCEPTION_EXECUTE_HANDLER ) {

        return STATUS_UNSUCCESSFUL;
    }
}

NTSTATUS
ZwReadFile(
    _In_    HANDLE           FileHandle,
    _In_    HANDLE           EventHandle,
    _Inout_ PIO_STATUS_BLOCK StatusBlock,
    _Out_   PVOID            Buffer,
    _In_    ULONG64          Length,
    _In_    ULONG64          Offset
)
{
    //
    // Implement SYNCHRONIZE/async
    // Implement io completion procedures.
    //

    NTSTATUS ntStatus;
    PIRP Request;
    PIO_FILE_OBJECT FileObject;
    PKEVENT Event;

    ntStatus = ObReferenceObjectByHandle( &FileObject,
                                          FileHandle,
                                          GENERIC_READ,
                                          KernelMode,
                                          IoFileObject );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    Event = NULL;

    if ( EventHandle != 0 ) {
        ntStatus = ObReferenceObjectByHandle( &Event,
                                              EventHandle,
                                              0,
                                              KernelMode,
                                              KeEventObject );
        if ( !NT_SUCCESS( ntStatus ) ) {

            ObDereferenceObject( FileObject );
            return ntStatus;
        }

        KeSignalEvent( Event, FALSE );
    }

    Request = IoBuildSynchronousFsdRequest( FileObject->DeviceObject,
                                            IRP_MJ_READ,
                                            Buffer,
                                            Length,
                                            Offset,
                                            Event,
                                            StatusBlock );
    Request->FileObject = FileObject;

    if ( FileObject->Access & SYNCHRONIZE ) {

        ntStatus = IoCallDriver( Request->DeviceObject, Request );

        ObDereferenceObject( Request->Process );
        ObDereferenceObject( Request->Thread );

        IoFreeIrp( Request );
        ObDereferenceObject( FileObject );
    }
    else {
        RtlDebugPrint( L"ASYNC READ REQUEST.\n" );
        IopEnqueueRequest( Request );
    }

    return ntStatus;
}

NTSTATUS
NtReadFile(
    _In_    HANDLE           FileHandle,
    _In_    HANDLE           EventHandle,
    _Inout_ PIO_STATUS_BLOCK StatusBlock,
    _Out_   PVOID            Buffer,
    _In_    ULONG64          Length,
    _In_    ULONG64          Offset
)
{
    __try {
        return ZwReadFile( FileHandle,
                           EventHandle,
                           StatusBlock,
                           Buffer,
                           Length,
                           Offset );
    }
    __except ( EXCEPTION_EXECUTE_HANDLER ) {

        return STATUS_UNSUCCESSFUL;
    }
}

NTSTATUS
ZwWriteFile(
    _In_    HANDLE           FileHandle,
    _In_    HANDLE           EventHandle,
    _Inout_ PIO_STATUS_BLOCK StatusBlock,
    _Out_   PVOID            Buffer,
    _In_    ULONG64          Length,
    _In_    ULONG64          Offset
)
{
    //
    // lol looks awfully similar to the function above
    //

    NTSTATUS ntStatus;
    PIRP Request;
    PIO_FILE_OBJECT FileObject;
    PKEVENT Event;

    ntStatus = ObReferenceObjectByHandle( &FileObject,
                                          FileHandle,
                                          GENERIC_WRITE,
                                          KernelMode,
                                          IoFileObject );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    Event = NULL;

    if ( EventHandle != 0 ) {
        ntStatus = ObReferenceObjectByHandle( &Event,
                                              EventHandle,
                                              0,
                                              KernelMode,
                                              KeEventObject );
        if ( !NT_SUCCESS( ntStatus ) ) {

            ObDereferenceObject( FileObject );
            return ntStatus;
        }

        KeSignalEvent( Event, FALSE );
    }

    Request = IoBuildSynchronousFsdRequest( FileObject->DeviceObject,
                                            IRP_MJ_WRITE,
                                            Buffer,
                                            Length,
                                            Offset,
                                            Event,
                                            StatusBlock );
    Request->FileObject = FileObject;

    if ( FileObject->Access & SYNCHRONIZE ) {

        ntStatus = IoCallDriver( Request->DeviceObject, Request );

        ObDereferenceObject( Request->Process );
        ObDereferenceObject( Request->Thread );

        IoFreeIrp( Request );
        ObDereferenceObject( FileObject );
    }
    else {
        RtlDebugPrint( L"ASYNC WRITE REQUEST.\n" );
        IopEnqueueRequest( Request );
    }

    return ntStatus;
}

NTSTATUS
NtWriteFile(
    _In_    HANDLE           FileHandle,
    _In_    HANDLE           EventHandle,
    _Inout_ PIO_STATUS_BLOCK StatusBlock,
    _Out_   PVOID            Buffer,
    _In_    ULONG64          Length,
    _In_    ULONG64          Offset
)
{
    __try {
        return ZwWriteFile( FileHandle,
                            EventHandle,
                            StatusBlock,
                            Buffer,
                            Length,
                            Offset );
    }
    __except ( EXCEPTION_EXECUTE_HANDLER ) {

        return STATUS_UNSUCCESSFUL;
    }
}

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
)
{

    NTSTATUS ntStatus;
    PIRP Request;
    PIO_FILE_OBJECT FileObject;
    PKEVENT Event;

    ntStatus = ObReferenceObjectByHandle( &FileObject,
                                          FileHandle,
                                          0,
                                          KernelMode,
                                          IoFileObject );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    Event = NULL;

    if ( EventHandle != 0 ) {
        ntStatus = ObReferenceObjectByHandle( &Event,
                                              EventHandle,
                                              0,
                                              KernelMode,
                                              KeEventObject );
        if ( !NT_SUCCESS( ntStatus ) ) {

            ObDereferenceObject( FileObject );
            return ntStatus;
        }

        KeSignalEvent( Event, FALSE );
    }

    Request = IoBuildDeviceIoControlRequest( FileObject->DeviceObject,
                                             ControlCode,
                                             InputBuffer,
                                             InputLength,
                                             OutputBuffer,
                                             OutputLength,
                                             Event,
                                             StatusBlock );
    Request->FileObject = FileObject;

    if ( FileObject->Access & SYNCHRONIZE ) {

        ntStatus = IoCallDriver( Request->DeviceObject, Request );

        ObDereferenceObject( Request->Process );
        ObDereferenceObject( Request->Thread );

        IoFreeIrp( Request );
        ObDereferenceObject( FileObject );
    }
    else {

        IopEnqueueRequest( Request );
    }

    return ntStatus;
}

NTSTATUS
NtDeviceIoControlFile(
    _In_  HANDLE           FileHandle,
    _In_  HANDLE           EventHandle,
    _Out_ PIO_STATUS_BLOCK StatusBlock,
    _In_  ULONG32          ControlCode,
    _Out_ PVOID            InputBuffer,
    _In_  ULONG64          InputLength,
    _In_  PVOID            OutputBuffer,
    _In_  ULONG64          OutputLength
)
{
    __try {
        return ZwDeviceIoControlFile( FileHandle,
                                      EventHandle,
                                      StatusBlock,
                                      ControlCode,
                                      InputBuffer,
                                      InputLength,
                                      OutputBuffer,
                                      OutputLength );
    }
    __except ( EXCEPTION_EXECUTE_HANDLER ) {

        return STATUS_UNSUCCESSFUL;
    }
}

NTSTATUS
ZwQueryInformationFile(
    _In_  HANDLE                 FileHandle,
    _Out_ PIO_STATUS_BLOCK       StatusBlock,
    _Out_ PVOID                  FileInformation,
    _In_  ULONG64                Length,
    _In_  FILE_INFORMATION_CLASS FileInformationClass
)
{
    NTSTATUS ntStatus;
    PIRP Request;
    PIO_FILE_OBJECT FileObject;

    ntStatus = ObReferenceObjectByHandle( &FileObject,
                                          FileHandle,
                                          0,
                                          KernelMode,
                                          IoFileObject );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    Request = IoAllocateIrp( FileObject->DeviceObject->StackLength );
    Request->Process = PsGetCurrentProcess( );
    Request->Thread = PsGetCurrentThread( );
    Request->RequestorMode = PsGetPreviousMode( Request->Thread );
    Request->DeviceObject = FileObject->DeviceObject;
    Request->FileObject = FileObject;

    ObReferenceObject( Request->Process );
    ObReferenceObject( Request->Thread );

    Request->SystemBuffer1 = FileInformation;

    Request->StackLocation[ 0 ].MajorFunction = IRP_MJ_QUERY_INFORMATION_FILE;
    Request->StackLocation[ 0 ].MinorFunction = 0;
    Request->StackLocation[ 0 ].Parameters.QueryFile.Info = FileInformationClass;
    Request->StackLocation[ 0 ].Parameters.QueryFile.Length = Length;

    Request->IoCompletion = IoCompletionRoutine;
    Request->User.Event = NULL;
    Request->User.IoStatus = StatusBlock;

    ntStatus = IoCallDriver( Request->DeviceObject, Request );

    ObDereferenceObject( Request->Process );
    ObDereferenceObject( Request->Thread );

    IoFreeIrp( Request );
    ObDereferenceObject( FileObject );

    return ntStatus;
}

NTSTATUS
NtQueryInformationFile(
    _In_  HANDLE                 FileHandle,
    _Out_ PIO_STATUS_BLOCK       StatusBlock,
    _Out_ PVOID                  FileInformation,
    _In_  ULONG64                Length,
    _In_  FILE_INFORMATION_CLASS FileInformationClass
)
{
    __try {

        return ZwQueryInformationFile( FileHandle,
                                       StatusBlock,
                                       FileInformation,
                                       Length,
                                       FileInformationClass );
    }
    __except ( EXCEPTION_EXECUTE_HANDLER ) {

        return STATUS_UNSUCCESSFUL;
    }
}

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
)
{
    //
    // If FileName != NULL - the caller only wants directory information
    // for that single listing regardless of SingleMode.
    //
    // If SingleMode - the caller wants directory information for the file
    // indentified by FileIndex.
    //

    NTSTATUS ntStatus;
    PIRP Request;
    PIO_FILE_OBJECT FileObject;

    ntStatus = ObReferenceObjectByHandle( &FileObject,
                                          FileHandle,
                                          0,
                                          KernelMode,
                                          IoFileObject );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    Request = IoAllocateIrp( FileObject->DeviceObject->StackLength );
    Request->Process = PsGetCurrentProcess( );
    Request->Thread = PsGetCurrentThread( );
    Request->RequestorMode = PsGetPreviousMode( Request->Thread );
    Request->DeviceObject = FileObject->DeviceObject;
    Request->FileObject = FileObject;

    ObReferenceObject( Request->Process );
    ObReferenceObject( Request->Thread );

    Request->SystemBuffer1 = FileInformation;
    Request->SystemBuffer2 = FileName;

    Request->StackLocation[ 0 ].MajorFunction = IRP_DIRECTORY_CONTROL;
    Request->StackLocation[ 0 ].MinorFunction = 0;
    Request->StackLocation[ 0 ].Parameters.DirectoryControl.Info = FileInformationClass;
    Request->StackLocation[ 0 ].Parameters.DirectoryControl.Length = Length;
    Request->StackLocation[ 0 ].Parameters.DirectoryControl.SingleMode = SingleMode;
    Request->StackLocation[ 0 ].Parameters.DirectoryControl.FileIndex = FileIndex;

    Request->IoCompletion = IoCompletionRoutine;
    Request->User.Event = NULL;
    Request->User.IoStatus = StatusBlock;

    ntStatus = IoCallDriver( Request->DeviceObject, Request );

    ObDereferenceObject( Request->Process );
    ObDereferenceObject( Request->Thread );

    IoFreeIrp( Request );
    ObDereferenceObject( FileObject );

    return ntStatus;
}

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
)
{
    __try {

        return ZwQueryDirectoryFile( FileHandle,
                                     StatusBlock,
                                     FileInformation,
                                     Length,
                                     FileInformationClass,
                                     FileName,
                                     FileIndex,
                                     SingleMode );
    }
    __except ( EXCEPTION_EXECUTE_HANDLER ) {

        return STATUS_UNSUCCESSFUL;
    }
}
