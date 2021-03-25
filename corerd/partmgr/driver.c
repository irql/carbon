


#include "driver.h"

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
    UNICODE_STRING Disk;
    ULONG64 DiskCount;

    RTL_STACK_STRING( Disk, 256 );

    DiskCount = FsRtlQueryDiskCount( );

    while ( DiskCount-- ) {

        RtlFormatBuffer( Disk.Buffer, L"\\Device\\Harddisk%d\\Drive", DiskCount );
        Disk.Length = ( USHORT )RtlStringLength( Disk.Buffer ) * sizeof( WCHAR );
        //RtlDebugPrint( L"[partmgr] setting up %s\n", Disk.Buffer );
        FsCreateMbrPartTable( DriverObject, &Disk );
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
    Request;
    DeviceObject;

    PIO_STACK_LOCATION Current;
    PIO_STACK_LOCATION Next;
    PPART_DEVICE Part;

    Current = IoGetCurrentStackLocation( Request );
    Next = IoGetNextStackLocation( Request );
    IoCopyCurrentIrpStackToNext( Request );

    Part = DeviceObject->DeviceExtension;

    switch ( Current->MajorFunction ) {
    case IRP_MJ_READ:
    case IRP_MJ_WRITE:
        if ( Next->Parameters.Read.Length > Part->SectorCount * 512 ||
             Next->Parameters.Read.Offset >= Part->SectorCount * 512 ) {

            Request->IoStatus.Status = STATUS_INVALID_ADDRESS;
            Request->IoStatus.Information = 0;
            IoCompleteRequest( Request );
        }
        else {

            Next->Parameters.Read.Offset += Part->StartLba * 512;
            //RtlDebugPrint( L"[partmgr] offset %d\n", Next->Parameters.Read.Offset / 512 );
        }

        break;
    default:

        break;
    }

    return STATUS_SUCCESS;
}
