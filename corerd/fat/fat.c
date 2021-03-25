


#include "fat.h"

//
// add error handling.
//

NTSTATUS
FspReadSectors(
    _In_ PDEVICE_OBJECT PartDevice,
    _In_ PVOID          Buffer,
    _In_ ULONG64        Length,
    _In_ ULONG64        Offset
)
{
    NTSTATUS ntStatus;
    PIRP Request;
    IO_STATUS_BLOCK Iosb;

    Request = IoBuildSynchronousFsdRequest( PartDevice,
                                            IRP_MJ_READ,
                                            Buffer,
                                            Length * 512,
                                            Offset * 512,
                                            NULL,
                                            &Iosb );

    ntStatus = IoCallDriver( PartDevice, Request );

    if ( !NT_SUCCESS( Iosb.Status ) ) {

        return Iosb.Status;
    }

    return ntStatus;
}

NTSTATUS
FspQueryFatTable(
    _In_  PDEVICE_OBJECT DeviceObject,
    _In_  ULONG32        Index,
    _Out_ ULONG32*       Next
)
{
    NTSTATUS ntStatus;
    PFAT_DEVICE Fat;
    ULONG32 FatSectorOffset;
    ULONG32 FatSectorIndex;
    ULONG32 FatTable[ 128 ];

    Fat = FspFatDevice( DeviceObject );

    FatSectorOffset = ( Index * 4 ) / 512;
    FatSectorIndex = ( ( Index * 4 ) % 512 ) / 4;

    ntStatus = FspReadSectors( DeviceObject->DeviceLink, &FatTable, 1, Fat->Bpb.Dos2_00Bpb.ReservedSectors + FatSectorOffset );

    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    *Next = FatTable[ FatSectorIndex ] & 0x0FFFFFFF;

    return STATUS_SUCCESS;
}