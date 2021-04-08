


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
FspWriteSectors(
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
                                            IRP_MJ_WRITE,
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
    ULONG32 FatSector;
    ULONG32 FatSectorIndex;
    ULONG32 FatTable[ 128 ];

    Fat = FspFatDevice( DeviceObject );

    FatSector = Index / 128;
    FatSectorIndex = Index % 128;

    ntStatus = FspReadSectors( DeviceObject->DeviceLink,
                               &FatTable,
                               1,
                               Fat->Bpb.Dos2_00Bpb.ReservedSectors + FatSector );

    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    *Next = FatTable[ FatSectorIndex ] & 0x0FFFFFFF;

    return STATUS_SUCCESS;
}

NTSTATUS
FspAllocateCluster(
    _In_  PDEVICE_OBJECT DeviceObject,
    _In_  ULONG32        Parent,
    _Out_ ULONG32*       Next
)
{
    //
    // This function finds a free cluster and makes the
    // Parent parameter on the fat table point to it.
    // The Cluster parameter is set to the new cluster
    // and this cluster points to end of chain.
    //

    NTSTATUS ntStatus;
    PFAT_DEVICE Fat;
    ULONG32 FatSector;
    ULONG32 FatSectorIndex;
    ULONG32 FatSectorParent;
    ULONG32 FatSectorIndexParent;
    ULONG32 FatTable[ 128 ];
    ULONG32 FatTableParent[ 128 ];

    Fat = FspFatDevice( DeviceObject );

    FatSector = Fat->FatHint / 128;
    FatSectorIndex = Fat->FatHint % 128;

    if ( Parent != 0 ) {

        FatSectorParent = Parent / 128;
        FatSectorIndexParent = Parent % 128;

        ntStatus = FspReadSectors( DeviceObject->DeviceLink,
                                   &FatTableParent,
                                   1,
                                   Fat->Bpb.Dos2_00Bpb.ReservedSectors + FatSectorParent );

        if ( !NT_SUCCESS( ntStatus ) ) {

            return ntStatus;
        }
    }

    while ( FatSector < Fat->Bpb.Dos7_01Bpb.SectorsPerFat ) {

        ntStatus = FspReadSectors( DeviceObject->DeviceLink,
                                   &FatTable,
                                   1,
                                   Fat->Bpb.Dos2_00Bpb.ReservedSectors + FatSector );

        if ( !NT_SUCCESS( ntStatus ) ) {

            return ntStatus;
        }

        for ( ; FatSectorIndex < 128; FatSectorIndex++ ) {

            if ( ( FatTable[ FatSectorIndex ] & 0x0FFFFFFF ) == 0 ) {

                if ( Parent != 0 ) {

                    FatTableParent[ FatSectorIndexParent ] &= 0x0FFFFFFF;
                    FatTableParent[ FatSectorIndexParent ] |= FatSector * 128 + FatSectorIndex;

                    if ( FatSectorParent == FatSector ) {

                        FatTableParent[ FatSectorIndex ] |= FAT32_END_OF_CHAIN;

                        FspWriteSectors( DeviceObject->DeviceLink,
                                         &FatTableParent,
                                         1,
                                         Fat->Bpb.Dos2_00Bpb.ReservedSectors + FatSectorParent );
                    }
                    else {
                        FspWriteSectors( DeviceObject->DeviceLink,
                                         &FatTableParent,
                                         1,
                                         Fat->Bpb.Dos2_00Bpb.ReservedSectors + FatSectorParent );

                        FatTable[ FatSectorIndex ] |= FAT32_END_OF_CHAIN;

                        FspWriteSectors( DeviceObject->DeviceLink,
                                         &FatTable,
                                         1,
                                         Fat->Bpb.Dos2_00Bpb.ReservedSectors + FatSector );
                    }
                }
                else {

                    FatTable[ FatSectorIndex ] |= FAT32_END_OF_CHAIN;

                    FspWriteSectors( DeviceObject->DeviceLink,
                                     &FatTable,
                                     1,
                                     Fat->Bpb.Dos2_00Bpb.ReservedSectors + FatSector );
                }

                *Next = FatSector * 128 + FatSectorIndex;

                Fat->FatHint = FatSector * 128 + FatSectorIndex + 1;
                return STATUS_SUCCESS;
            }
        }

        FatSectorIndex = 0;
    }

    return STATUS_NOT_FOUND;
}

NTSTATUS
FspResizeChain(
    _In_ PDEVICE_OBJECT  DeviceObject,
    _In_ PFAT_FILE_CHAIN Chain,
    _In_ ULONG32         ChainLength
)
{
    NTSTATUS ntStatus;
    ULONG32* NewChain;
    ULONG32* OldChain;
    ULONG64 CurrentEntry;

    if ( Chain->ChainLength == ChainLength ) {

        return STATUS_SUCCESS;
    }

    if ( Chain->ChainLength > ChainLength ) {

        //
        // Decreasing chain size.
        //

        NT_ASSERT( FALSE );
    }
    else {

        //
        // Increasing chain size.
        //

        NewChain = MmAllocatePoolWithTag( NonPagedPool,
            ( ChainLength + Chain->ChainLength ) * sizeof( ULONG32 ),
                                          FAT_TAG );
        OldChain = Chain->Chain;

        if ( Chain->ChainLength > 0 ) {

            RtlCopyMemory( NewChain, OldChain, Chain->ChainLength );
        }

        ChainLength -= Chain->ChainLength;

        for ( CurrentEntry = 0; CurrentEntry < ChainLength; CurrentEntry++ ) {

            ntStatus = FspAllocateCluster( DeviceObject,
                                           Chain->ChainLength + CurrentEntry > 0 ? NewChain[ Chain->ChainLength + CurrentEntry - 1 ] : 0,
                                           &NewChain[ Chain->ChainLength + CurrentEntry ] );
            if ( !NT_SUCCESS( ntStatus ) ) {

                //
                // TODO !!! : must go back and free all clusters added.
                //

                MmFreePoolWithTag( NewChain, FAT_TAG );
                return ntStatus;
            }
        }

        if ( Chain->ChainLength > 0 ) {

            MmFreePoolWithTag( OldChain, FAT_TAG );
        }

        Chain->Chain = NewChain;
        Chain->ChainLength += ChainLength;
    }

    return STATUS_SUCCESS;
}
