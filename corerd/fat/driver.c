


#include "fat.h"

NTSTATUS
DriverLoad(
    _In_ PDRIVER_OBJECT DriverObject
);

NTSTATUS
DriverDispatch(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP           Request
);

NTSTATUS
DriverLoad(
    _In_ PDRIVER_OBJECT DriverObject
)
{

    NTSTATUS ntStatus;
    HANDLE FileHandle;
    IO_STATUS_BLOCK Iosb;
    OBJECT_ATTRIBUTES ObjectAttributes = { 0 };
    BIOS_PARAMETER_BLOCK Bpb;
    PKDISK_HEADER Header;
    ULONG64 DiskCount;
    ULONG64 CurrentPart;
    UNICODE_STRING DriveLetterName;
    UNICODE_STRING BootDevice = RTL_CONSTANT_STRING( L"\\??\\BootDevice" );
    UNICODE_STRING DiskName;
    PDEVICE_OBJECT FsDevice;
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_OBJECT PartDevice;
    PFAT_DEVICE Fat;

    DriveLetterName.Buffer = MmAllocatePoolWithTag( NonPagedPool, 256, FAT_TAG );
    DriveLetterName.MaximumLength = 256;
    ObjectAttributes.ObjectName.Buffer = MmAllocatePoolWithTag( NonPagedPool, 256, FAT_TAG );
    ObjectAttributes.ObjectName.MaximumLength = 256;
    DiskName.Buffer = ObjectAttributes.ObjectName.Buffer;
    DiskName.MaximumLength = 256;

    DiskCount = FsRtlQueryDiskCount( );

    while ( DiskCount-- ) {

        RtlFormatBuffer( DiskName.Buffer, L"\\Device\\Harddisk%d\\Drive", DiskCount );
        DiskName.Length = ( USHORT )RtlStringLength( DiskName.Buffer ) * sizeof( WCHAR );
        //RtlDebugPrint( L"[fat] checking disk: %s\n", DiskName.Buffer );

        ntStatus = ObReferenceObjectByName( &DeviceObject, &DiskName, IoDeviceObject );
        if ( !NT_SUCCESS( ntStatus ) ) {

            continue;
        }

        Header = DeviceObject->DeviceExtension;

        if ( Header->PartCount == 0 ) {

            //
            // Allow file systems to be mounted when they dont have any partitioning.
            //

            continue;
        }

        for ( CurrentPart = 0; CurrentPart < Header->PartCount; CurrentPart++ ) {

            RtlFormatBuffer( ObjectAttributes.ObjectName.Buffer, L"\\Device\\Harddisk%d\\Partition%d", DiskCount, CurrentPart );
            ObjectAttributes.ObjectName.Length = ( USHORT )RtlStringLength( ObjectAttributes.ObjectName.Buffer ) * sizeof( WCHAR );
#if 0
            RtlDebugPrint( L"[fat] checking part: %s %s\n",
                           ObjectAttributes.ObjectName.Buffer,
                           ObjectAttributes.RootDirectory.Buffer );
#endif
            ObReferenceObjectByName( &PartDevice,
                                     &ObjectAttributes.ObjectName,
                                     IoDeviceObject );
#if 0
            RtlDebugPrint( L"part device at %ull %ull\n", PartDevice,
                           &PartDevice->DriverObject->MajorFunction[ 0 ] );
#endif
            //
            // TODO: Check if the partition is FAT32, if it is, we should attach our own device.
            //

            ntStatus = ZwCreateFile( &FileHandle,
                                     &Iosb,
                                     GENERIC_ALL | SYNCHRONIZE,
                                     &ObjectAttributes,
                                     FILE_OPEN_IF,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                                     0 );

            if ( !NT_SUCCESS( ntStatus ) ) {

                continue;
            }

            if ( !NT_SUCCESS( Iosb.Status ) ) {

                ZwClose( FileHandle );
                continue;
            }

            ntStatus = ZwReadFile( FileHandle,
                                   0,
                                   &Iosb,
                                   &Bpb,
                                   512,
                                   0 );

            if ( !NT_SUCCESS( ntStatus ) ||
                 !NT_SUCCESS( Iosb.Status ) ) {

                ZwClose( FileHandle );
                continue;
            }

            RtlFormatBuffer( DriveLetterName.Buffer, L"\\??\\%ac:", FsRtlNextDriveLetter( ) );
            DriveLetterName.Length = ( USHORT )RtlStringLength( DriveLetterName.Buffer ) * sizeof( WCHAR );
            //RtlDebugPrint( L"[fat] fat32 partition: %s\n", DriveLetterName.Buffer );

            IoCreateDevice( DriverObject,
                            sizeof( FAT_DEVICE ),
                            &DriveLetterName,
                            DEV_BUFFERED_IO,
                            &FsDevice );
            IoAttachDevice( PartDevice, FsDevice );

            Fat = FspFatDevice( FsDevice );
            RtlCopyMemory( &Fat->Bpb, &Bpb, sizeof( BIOS_PARAMETER_BLOCK ) );
            Fat->Root = MmAllocatePoolWithTag( NonPagedPool, 512 * Fat->Bpb.Dos2_00Bpb.SectorsPerCluster, FAT_TAG );
            Fat->RootStatus = FspReadSectors( PartDevice,
                                              Fat->Root,
                                              Fat->Bpb.Dos2_00Bpb.SectorsPerCluster,
                                              FIRST_SECTOR_OF_CLUSTER( &Fat->Bpb, Fat->Bpb.Dos7_01Bpb.RootDirectoryCluster ) );

            if ( KeRootSerial == Bpb.Dos7_01Bpb.VolumeSerialNumber ) {

                //RtlDebugPrint( L"[fat] created boot link. %s -> %s\n", BootDevice.Buffer, DriveLetterName.Buffer );
                IoCreateSymbolicLink( &BootDevice, &DriveLetterName );
            }

            ZwClose( FileHandle );
            ObDereferenceObject( PartDevice );
        }

        ObDereferenceObject( DeviceObject );
    }

    __stosq( ( ULONG64* )&DriverObject->MajorFunction, ( ULONG64 )DriverDispatch, IRP_MJ_MAX );

    return STATUS_SUCCESS;
}

NTSTATUS
DriverDispatch(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP           Request
)
{
    DeviceObject;
    PIO_STACK_LOCATION Current;
    PFAT_DEVICE Fat;
    PFAT_FILE_CONTEXT File;
    PFAT_DIRECTORY Directory;
    NTSTATUS ntStatus;

    //RtlDebugPrint( L"[fat] dispatch.\n" );

    Current = IoGetCurrentStackLocation( Request );
    IoCopyCurrentIrpStackToNext( Request );

    Fat = FspFatDevice( DeviceObject );
    File = FspFatFileContext( Request->FileObject );
    if ( !NT_SUCCESS( Fat->RootStatus ) ) {

        Request->IoStatus.Status = STATUS_INACCESSIBLE_DEVICE;
        Request->IoStatus.Information = 0;
        IoCompleteRequest( Request );
        return STATUS_SUCCESS;
    }

    switch ( Current->MajorFunction ) {
    case IRP_MJ_CREATE:

        Request->FileObject->FsContext1 = MmAllocatePoolWithTag( NonPagedPool,
                                                                 sizeof( FAT_FILE_CONTEXT ),
                                                                 FAT_TAG );
        Request->IoStatus.Status = FsOpenFat32File( DeviceObject, Request );
        break;
    case IRP_MJ_READ:

        if ( ( File->Flags & FILE_FLAG_DIRECTORY ) != 0 ) {

            Request->IoStatus.Status = STATUS_INVALID_PATH;
            Request->IoStatus.Information = 0;
            break;
        }

        Request->IoStatus.Status = FspReadChain( DeviceObject,
                                                 File->Chain,
                                                 Request->SystemBuffer1,
                                                 Current->Parameters.Read.Length,
                                                 Current->Parameters.Read.Offset );
        Request->IoStatus.Information = Current->Parameters.Read.Length;
        break;
    case IRP_MJ_WRITE:

        break;
    case IRP_MJ_QUERY_INFORMATION_FILE:

        switch ( Current->Parameters.QueryFile.Info ) {
        case FileBasicInformation:;
            PFILE_BASIC_INFORMATION Basic = Request->SystemBuffer1;

            Basic->FileLength = Request->FileObject->FileLength;

            Request->IoStatus.Status = STATUS_SUCCESS;
            Request->IoStatus.Information = sizeof( FILE_BASIC_INFORMATION );
            break;
        default:

            Request->IoStatus.Status = STATUS_INVALID_REQUEST;
            Request->IoStatus.Information = 0;

            break;
        }

        break;
    case IRP_DIRECTORY_CONTROL:

        switch ( Current->Parameters.DirectoryControl.Info ) {
        case FileDirectoryInformation:;

            if ( ( File->Flags & FILE_FLAG_DIRECTORY ) == 0 ) {

                Request->IoStatus.Status = STATUS_INVALID_PATH;
                Request->IoStatus.Information = 0;
                break;
            }

            Directory = MmAllocatePoolWithTag( NonPagedPool,
                                               512 * Fat->Bpb.Dos2_00Bpb.SectorsPerCluster,
                                               FAT_TAG );

            ntStatus = FspReadChain( DeviceObject,
                                     File->Chain,
                                     Directory,
                                     512 * Fat->Bpb.Dos2_00Bpb.SectorsPerCluster,
                                     0 );

            if ( !NT_SUCCESS( ntStatus ) ) {

                Request->IoStatus.Status = ntStatus;
                Request->IoStatus.Information = 0;
                break;
            }

            if ( Request->SystemBuffer2 == NULL &&
                 Current->Parameters.DirectoryControl.SingleMode ) {

                Request->IoStatus.Status = FsQueryIndexFile( DeviceObject,
                                                             Directory,
                                                             Current->Parameters.DirectoryControl.FileIndex,
                                                             Current->Parameters.DirectoryControl.Length,
                                                             Request->SystemBuffer1,
                                                             &Request->IoStatus.Information );
            }
            else if ( Request->SystemBuffer2 != NULL ) {
                Request->IoStatus.Status = FsQueryNameFile( DeviceObject,
                                                            Directory,
                                                            Request->SystemBuffer2,
                                                            Current->Parameters.DirectoryControl.Length,
                                                            Request->SystemBuffer1,
                                                            &Request->IoStatus.Information );
            }
            else {

                NT_ASSERT( FALSE );
            }

            MmFreePoolWithTag( Directory, FAT_TAG );

            break;
        default:

            Request->IoStatus.Status = STATUS_INVALID_REQUEST;
            Request->IoStatus.Information = 0;

            break;
        }

        break;
    case IRP_MJ_CLOSE:

        Request->IoStatus.Status = STATUS_SUCCESS;
        Request->IoStatus.Information = 0;

        break;
    case IRP_MJ_CLEANUP:

        MmFreePoolWithTag( Request->FileObject->FsContext1, FAT_TAG );
        Request->IoStatus.Status = STATUS_SUCCESS;
        Request->IoStatus.Information = 0;

        break;
    default:

        Request->IoStatus.Status = STATUS_INVALID_REQUEST;
        Request->IoStatus.Information = 0;

        break;
    }

    IoCompleteRequest( Request );

    return STATUS_SUCCESS;
}
