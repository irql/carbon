


#include "driver.h"

#define PARTITION_NAME  L"Partition%d"

PDEVICE_OBJECT
FsCreatePartition(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PDEVICE_OBJECT  DiskDevice,
    _In_ PUNICODE_STRING DiskDirectory,
    _In_ ULONG64         PartitionNumber,
    _In_ PUNICODE_STRING PartitionDirectory
)
{
    PDEVICE_OBJECT PartDevice;

    PartitionDirectory->Buffer = MmAllocatePoolWithTag( NonPagedPoolZeroed, 256, PART_TAG );
    PartitionDirectory->Length = 0;
    PartitionDirectory->MaximumLength = 256;
    RtlFormatBuffer( PartitionDirectory->Buffer, L"%s\\" PARTITION_NAME, DiskDirectory->Buffer, PartitionNumber );
    PartitionDirectory->Length = ( USHORT )( RtlStringLength( PartitionDirectory->Buffer ) * sizeof( WCHAR ) );

    IoCreateDevice( DriverObject,
                    sizeof( PART_DEVICE ),
                    PartitionDirectory,
                    0,
                    &PartDevice );

    IoAttachDevice( DiskDevice, PartDevice );

    return PartDevice;
}

NTSTATUS
FsCreateMbrPartTable(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING Disk
)
{
    NTSTATUS ntStatus;
    HANDLE FileHandle;
    PART_MBR BootRecord;
    UNICODE_STRING DiskDirectory;
    UNICODE_STRING PartDirectory;
    OBJECT_ATTRIBUTES ObjectAttributes = { { 0 }, { 0 }, OBJ_KERNEL_HANDLE };
    IO_STATUS_BLOCK Iosb;
    ULONG64 Entry;
    ULONG64 PartNumber;
    PDEVICE_OBJECT DeviceObject;
    PPART_DEVICE PartDevice;
    PIO_FILE_OBJECT FileObject;

    PartNumber = 0;
    DiskDirectory.Buffer = MmAllocatePoolWithTag( NonPagedPool, 512, PART_TAG );
    DiskDirectory.MaximumLength = 512;

    RtlCopyMemory( &ObjectAttributes.ObjectName, Disk, sizeof( UNICODE_STRING ) );

    FsRtlContainingDirectory( Disk, &DiskDirectory );

    ntStatus = ZwCreateFile( &FileHandle,
                             &Iosb,
                             GENERIC_ALL | SYNCHRONIZE,
                             &ObjectAttributes,
                             FILE_OPEN_IF,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             0 );

    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    ntStatus = ZwReadFile( FileHandle,
                           0,
                           &Iosb,
                           &BootRecord,
                           512,
                           0 );

    if ( !NT_SUCCESS( ntStatus ) ||
         !NT_SUCCESS( Iosb.Status ) ) {

        ZwClose( FileHandle );
        return ntStatus;
    }

    ntStatus = ObReferenceObjectByHandle( &FileObject,
                                          FileHandle,
                                          0,
                                          KernelMode,
                                          IoFileObject );
    if ( !NT_SUCCESS( ntStatus ) ) {

        ZwClose( FileHandle );
        return ntStatus;
    }

    ( ( PDISK_OBJECT_HEADER )FileObject->DeviceObject->DeviceExtension )->PartCount = 0;

    for ( Entry = 0; Entry < 4; Entry++ ) {

        if ( BootRecord.Table[ Entry ].StartLba == 0 ||
             BootRecord.Table[ Entry ].SectorCount == 0 ) {

            continue;
        }

        DeviceObject = FsCreatePartition( DriverObject,
                                          FileObject->DeviceObject,
                                          &DiskDirectory,
                                          PartNumber,
                                          &PartDirectory );
        PartDevice = DeviceObject->DeviceExtension;
        PartDevice->StartLba = BootRecord.Table[ Entry ].StartLba;
        PartDevice->SectorCount = BootRecord.Table[ Entry ].SectorCount;

        ZwReadFile( FileHandle,
                    0,
                    &Iosb,
                    &PartDevice->BootSector,
                    512,
                    PartDevice->StartLba * 512 );
        //PartDevice->SerialNumber = ( ( PBIOS_PARAMETER_BLOCK )&PartDevice->BootSector )->Dos7_01Bpb.VolumeSerialNumber;

        //if ( BootRecord.Table[ Entry ].Attributes & MBR_ATTRIBUTE_ACTIVE &&
        // add flag for active drive.

#if 0
        RtlDebugPrint( L"created partition %d %d %d\n",
                       PartNumber,
                       PartDevice->StartLba,
                       PartDevice->SectorCount );

        RtlDebugPrint( L"[partmgr] %s\n", FileObject->DeviceObject->DriverObject->DriverName.Buffer );
#endif
        PartNumber++;
    }

    ( ( PDISK_OBJECT_HEADER )FileObject->DeviceObject->DeviceExtension )->PartCount = PartNumber;
    ObDereferenceObject( FileObject );
    ZwClose( FileHandle );

    return STATUS_SUCCESS;

}
