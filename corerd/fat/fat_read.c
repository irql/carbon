


#include "fat.h"

//
// change the chain stuff to use less allocs
//

NTSTATUS
FspReadChain(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ ULONG32*       Chain,
    _In_ PVOID          Buffer,
    _In_ ULONG64        Length,
    _In_ ULONG64        Offset
)
{
    //
    // reads the cluster chain in the file object
    //

    PFAT_DEVICE Fat;
    ULONG64 ChainIndex;
    PVOID SystemBuffer;

    Fat = FspFatDevice( DeviceObject );
    SystemBuffer = MmAllocatePoolWithTag( NonPagedPool, Fat->Bpb.Dos2_00Bpb.SectorsPerCluster * 512, FAT_TAG );
    ChainIndex = Offset / ( ( ULONG64 )Fat->Bpb.Dos2_00Bpb.SectorsPerCluster * 512 );
    Offset %= ( ULONG64 )Fat->Bpb.Dos2_00Bpb.SectorsPerCluster * 512;

    while ( Length > 0 ) {

        FspReadSectors( DeviceObject->DeviceLink,
                        SystemBuffer,
                        Fat->Bpb.Dos2_00Bpb.SectorsPerCluster,
                        FIRST_SECTOR_OF_CLUSTER( &Fat->Bpb, Chain[ ChainIndex ] ) );

        if ( Length > ( ULONG64 )Fat->Bpb.Dos2_00Bpb.SectorsPerCluster * 512 - Offset ) {

            RtlCopyMemory( Buffer, ( PUCHAR )SystemBuffer + Offset, Fat->Bpb.Dos2_00Bpb.SectorsPerCluster * 512 - Offset );
            Length -= ( ULONG64 )Fat->Bpb.Dos2_00Bpb.SectorsPerCluster * 512 - Offset;
            Buffer = ( PUCHAR )Buffer + ( ULONG64 )Fat->Bpb.Dos2_00Bpb.SectorsPerCluster * 512 - Offset;
            Offset = 0;
        }
        else {

            RtlCopyMemory( Buffer, ( PUCHAR )SystemBuffer + Offset, Length );
            break;
        }

        ChainIndex++;
    }

    MmFreePoolWithTag( SystemBuffer, FAT_TAG );

    return STATUS_SUCCESS;
}

NTSTATUS
FspBuildChain(
    _In_    PDEVICE_OBJECT DeviceObject,
    _In_    ULONG32        Start,
    _Inout_ ULONG32**      Chain,
    _Inout_ ULONG64*       ChainLength
)
{
    NTSTATUS ntStatus;
    ULONG32 CurrentCluster;
    ULONG64 ClusterCount;

    ClusterCount = 0;
    CurrentCluster = Start;
    do {
        ClusterCount++;
        ntStatus = FspQueryFatTable( DeviceObject, CurrentCluster, &CurrentCluster );
        if ( !NT_SUCCESS( ntStatus ) ) {

            return ntStatus;
        }
    } while ( CurrentCluster != FAT32_END_OF_CHAIN );

    *Chain = MmAllocatePoolWithTag( NonPagedPoolZeroed, ClusterCount * sizeof( ULONG32 ), FAT_TAG );
    *ChainLength = ClusterCount;

    CurrentCluster = Start;
    ClusterCount = 0;
    do {
        ( *Chain )[ ClusterCount++ ] = CurrentCluster;
        ntStatus = FspQueryFatTable( DeviceObject, CurrentCluster, &CurrentCluster );
        if ( !NT_SUCCESS( ntStatus ) ) {

            MmFreePoolWithTag( *Chain, FAT_TAG );
            return ntStatus;
        }
    } while ( CurrentCluster != FAT32_END_OF_CHAIN );

    return STATUS_SUCCESS;
}

NTSTATUS
FspBuildChainFromFile(
    _In_    PDEVICE_OBJECT DeviceObject,
    _In_    PIRP           Request,
    _In_    PFAT_DIRECTORY Directory,
    _In_    PVOID          FileName,
    _Inout_ ULONG32**      Chain,
    _Inout_ ULONG64*       ChainLength
)
{
    FAT_PATH_TYPE Type;
    CHAR FileName8Dot3[ 12 ];
    PWCHAR FileNameLfn;
    ULONG64 FatFile;
    PFAT_FILE_CONTEXT File;

    File = FspFatFileContext( Request->FileObject );
    Type = FspValidateFileName( FileName );

    if ( Type == Path8Dot3 ) {

        FspConvertPathTo8Dot3( FileName, FileName8Dot3 );

        FatFile = FspFindDirectoryFile( Directory, Type, FileName8Dot3 );
    }
    else if ( Type == PathLongFileName ) {

        FileNameLfn = FileName;
        FatFile = FspFindDirectoryFile( Directory, Type, FileNameLfn );
    }
    else {

        return STATUS_INVALID_PATH;
    }

    if ( FatFile == ( ULONG64 )-1 ) {

        return STATUS_NOT_FOUND;
    }

    Request->FileObject->FileLength = Directory[ FatFile ].Short.FileSize;

    if ( Directory[ FatFile ].Short.Attributes & FAT32_DIRECTORY )
        File->Flags |= FILE_FLAG_DIRECTORY;
    if ( Directory[ FatFile ].Short.Attributes & FAT32_HIDDEN )
        File->Flags |= FILE_FLAG_ATTRIBUTE_HIDDEN;
    if ( Directory[ FatFile ].Short.Attributes & FAT32_SYSTEM )
        File->Flags |= FILE_FLAG_ATTRIBUTE_SYSTEM;
    if ( Directory[ FatFile ].Short.Attributes & FAT32_READ_ONLY )
        File->Flags |= FILE_FLAG_ATTRIBUTE_READONLY;

    FspBuildChain( DeviceObject,
        ( ULONG32 )Directory[ FatFile ].Short.ClusterHigh << 16 | Directory[ FatFile ].Short.ClusterLow,
                   Chain,
                   ChainLength );

    return STATUS_SUCCESS;
}

NTSTATUS
FsOpenFat32File(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP           Request
)
{
    DeviceObject;
    //
    // This should create a file object for the
    // directory and fill in this one for the
    // file.
    //

    NTSTATUS ntStatus;
    PWSTR* Decomposed;
    ULONG64 Part;
    PFAT_DIRECTORY Directory;
    PFAT_FILE_CONTEXT File;
    PFAT_DEVICE Fat;
    ULONG32* Chain;
    ULONG64 ChainLength;
    UNICODE_STRING FileName;
    RTL_STACK_STRING( FileName, 256 );

    Fat = FspFatDevice( DeviceObject );
    File = FspFatFileContext( Request->FileObject );

    RtlCopyMemory( FileName.Buffer, Request->FileObject->FileName.Buffer, Request->FileObject->FileName.Length + 2 );
    FileName.Length = Request->FileObject->FileName.Length;

    ntStatus = ObpDecomposeDirectory( &FileName,
                                      &Decomposed );

    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    for ( Part = 0; Decomposed[ Part ] != NULL; Part++ ) {

        if ( FspValidateFileName( Decomposed[ Part ] ) == PathInvalid ) {

            MmFreePoolWithTag( Decomposed, '  bO' );
            return STATUS_INVALID_PATH;
        }
    }

    //cache this.
    Directory = MmAllocatePoolWithTag( NonPagedPool, 512 * Fat->Bpb.Dos2_00Bpb.SectorsPerCluster, FAT_TAG );
    RtlCopyMemory( Directory, Fat->Root, 512 * Fat->Bpb.Dos2_00Bpb.SectorsPerCluster );

    if ( !NT_SUCCESS( ntStatus ) ) {

        MmFreePoolWithTag( Decomposed, '  bO' );
        return STATUS_UNSUCCESSFUL;
    }

    for ( Part = 0; Decomposed[ Part ] != NULL; Part++ ) {

        if ( Decomposed[ Part + 1 ] == NULL ) {

            //
            // This is the last file in the path parsing
            //

            ntStatus = FspBuildChainFromFile( DeviceObject,
                                              Request,
                                              Directory,
                                              Decomposed[ Part ],
                                              &File->Chain,
                                              &File->ChainLength );

            if ( !NT_SUCCESS( ntStatus ) ) {

                MmFreePoolWithTag( Directory, FAT_TAG );
                return ntStatus;
            }
        }
        else {

            //
            // This is for any subdirectories,
            // we should build a chain and read it.
            //
            // TODO: Optimize chain building.
            // memory allocations are fairly costly.
            //

            ntStatus = FspBuildChainFromFile( DeviceObject,
                                              Request,
                                              Directory,
                                              Decomposed[ Part ],
                                              &Chain,
                                              &ChainLength );

            if ( !NT_SUCCESS( ntStatus ) ) {

                MmFreePoolWithTag( Directory, FAT_TAG );
                return ntStatus;
            }

            ntStatus = FspReadChain( DeviceObject,
                                     Chain,
                                     Directory,
                                     Fat->Bpb.Dos2_00Bpb.SectorsPerCluster * 512,
                                     0u );

            MmFreePoolWithTag( Chain, FAT_TAG );

            if ( !NT_SUCCESS( ntStatus ) ) {

                MmFreePoolWithTag( Directory, FAT_TAG );
                return ntStatus;
            }
        }
    }

    MmFreePoolWithTag( Directory, FAT_TAG );
    Request->IoStatus.Information = FILE_OPENED;
    return STATUS_SUCCESS;
}
