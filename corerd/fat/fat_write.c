


#include "fat.h"

ULONG64
FsFindFreeDirectoryFile(
    _In_ PFAT_DIRECTORY Directory
)
{
    ULONG64 Entry;

    for ( Entry = 0; Directory[ Entry ].Short.Name[ 0 ] != 0; Entry++ ) {

        if ( ( UCHAR )Directory[ Entry ].Short.Name[ 0 ] == ( UCHAR )FAT32_DIRECTORY_ENTRY_FREE ) {

            return Entry;
        }
    }

    return Entry;
}

ULONG64
FsFindLastDirectoryFile(
    _In_ PFAT_DIRECTORY Directory
)
{
    ULONG64 Entry;

    Entry = 0;

    while ( Directory[ Entry ].Short.Name[ 0 ] != 0 )
        Entry++;

    return Entry;
}

NTSTATUS
FsCreateFat32DirectoryFile(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP           Request
)
{
    //
    // I copy pasted FsOpenFat32File and renamed it.
    //

    NTSTATUS ntStatus;
    PWSTR* Decomposed;
    ULONG64 Part;
    PFAT_DIRECTORY Directory;
    UNICODE_STRING FileName;
    FAT_FILE_CHAIN DirectoryChain;
    ULONG64 FreeEntry;
    FAT_PATH_TYPE Type;
    ULONG64 Length;
    ULONG64 Short;

    PFAT_FILE_CONTEXT File;
    PFAT_DEVICE Fat;

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

    if ( Decomposed[ 0 ] == NULL ) {

        return STATUS_INVALID_PATH;
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

    for ( Part = 0; Decomposed[ Part ] != NULL; Part++ ) {

        if ( Decomposed[ Part + 1 ] == NULL ) {

            //
            // This is the last file in the path parsing
            //

            Type = FspValidateFileName( Decomposed[ Part ] );

            if ( Type == Path8Dot3 ) {

                FreeEntry = FsFindFreeDirectoryFile( Directory );

                FspConvertPathTo8Dot3( Decomposed[ Part ],
                                       Directory[ FreeEntry ].Short.Name );
                Directory[ FreeEntry ].Short.FileSize = 0;
                // warning: cast truncates constant value - shut the fuck up.
                Directory[ FreeEntry ].Short.ClusterLow = FAT32_END_OF_CHAIN & 0xFFFF;
                Directory[ FreeEntry ].Short.ClusterHigh = ( FAT32_END_OF_CHAIN >> 16 ) & 0xFFFF;

                Directory[ FreeEntry ].Short.Attributes = 0;
                if ( File->Flags & FILE_FLAG_DIRECTORY )
                    Directory[ FreeEntry ].Short.Attributes |= FAT32_DIRECTORY;

            }
            else {

                // not in the mood
                NT_ASSERT( FALSE );

                Length = RtlStringLength( Decomposed[ Part ] );
                Short = Length / 13 + ( Length % 13 != 0 );
                FreeEntry = FsFindLastDirectoryFile( Directory );

                Directory[ FreeEntry + Short ].Short.FileSize = 0;
                Directory[ FreeEntry + Short ].Short.ClusterLow = FAT32_END_OF_CHAIN & 0xFFFF;
                Directory[ FreeEntry + Short ].Short.ClusterHigh = ( FAT32_END_OF_CHAIN >> 16 ) & 0xFFFF;

                Directory[ FreeEntry + Short ].Short.Attributes = 0;
                if ( File->Flags & FILE_FLAG_DIRECTORY )
                    Directory[ FreeEntry + Short ].Short.Attributes |= FAT32_DIRECTORY;
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
                                              &DirectoryChain );

            if ( !NT_SUCCESS( ntStatus ) ) {

                MmFreePoolWithTag( Directory, FAT_TAG );
                return ntStatus;
            }

            if ( ( File->Flags & FILE_FLAG_DIRECTORY ) == 0 ) {

                MmFreePoolWithTag( Directory, FAT_TAG );
                return STATUS_INVALID_PATH;
            }

            ntStatus = FspReadChain( DeviceObject,
                                     &DirectoryChain,
                                     Directory,
                                     Fat->Bpb.Dos2_00Bpb.SectorsPerCluster * 512,
                                     0u );

            MmFreePoolWithTag( DirectoryChain.Chain, FAT_TAG );

            if ( !NT_SUCCESS( ntStatus ) ) {

                MmFreePoolWithTag( Directory, FAT_TAG );
                return ntStatus;
            }
        }
    }

    MmFreePoolWithTag( Directory, FAT_TAG );
    Request->IoStatus.Information = FILE_CREATED;
    return STATUS_SUCCESS;
}

NTSTATUS
FspWriteChain(
    _In_ PDEVICE_OBJECT  DeviceObject,
    _In_ PFAT_FILE_CHAIN Chain,
    _In_ PVOID           Buffer,
    _In_ ULONG64         Length,
    _In_ ULONG64         Offset
)
{
    //NTSTATUS ntStatus;
    PFAT_DEVICE Fat;
    ULONG32 ChainIndex;
    PVOID SystemBuffer;
    ULONG32 ChainLength;

    Fat = FspFatDevice( DeviceObject );

    SystemBuffer = MmAllocatePoolWithTag( NonPagedPool,
                                          Fat->Bpb.Dos2_00Bpb.SectorsPerCluster * 512,
                                          FAT_TAG );
    ChainLength = ( ULONG32 )(
        ( Offset + Length ) / ( ( ULONG32 )Fat->Bpb.Dos2_00Bpb.SectorsPerCluster * 512 ) +
        ( ( Offset + Length ) % ( ( ULONG32 )Fat->Bpb.Dos2_00Bpb.SectorsPerCluster * 512 ) != 0 ) );
    ChainIndex = ( ULONG32 )( Offset / ( ( ULONG32 )Fat->Bpb.Dos2_00Bpb.SectorsPerCluster * 512 ) );
    Offset %= ( ULONG64 )Fat->Bpb.Dos2_00Bpb.SectorsPerCluster * 512;

    if ( ChainLength > Chain->ChainLength ) {

        //
        // BUG: if the directory file points to no cluster, then this function will not
        // set the parent cluster properly (well it cant, because it doesnt exist)
        //

        FspResizeChain( DeviceObject,
                        Chain,
                        ChainLength );
    }

    while ( Length > 0 ) {

        if ( Offset != 0 ||
             Length < ( ULONG64 )Fat->Bpb.Dos2_00Bpb.SectorsPerCluster * 512 - Offset ) {

            FspReadSectors( DeviceObject->DeviceLink,
                            SystemBuffer,
                            Fat->Bpb.Dos2_00Bpb.SectorsPerCluster,
                            FIRST_SECTOR_OF_CLUSTER( &Fat->Bpb, Chain->Chain[ ChainIndex ] ) );

            RtlCopyMemory( ( PUCHAR )SystemBuffer + Offset,
                           Buffer,
                           min( Fat->Bpb.Dos2_00Bpb.SectorsPerCluster * 512 - Offset, Length ) );

            FspWriteSectors( DeviceObject->DeviceLink,
                             SystemBuffer,
                             Fat->Bpb.Dos2_00Bpb.SectorsPerCluster,
                             FIRST_SECTOR_OF_CLUSTER( &Fat->Bpb, Chain->Chain[ ChainIndex ] ) );
        }
        else {

            FspWriteSectors( DeviceObject->DeviceLink,
                             Buffer,
                             Fat->Bpb.Dos2_00Bpb.SectorsPerCluster,
                             FIRST_SECTOR_OF_CLUSTER( &Fat->Bpb, Chain->Chain[ ChainIndex ] ) );
        }

        if ( Length > ( ULONG64 )Fat->Bpb.Dos2_00Bpb.SectorsPerCluster * 512 - Offset ) {

            Length -= ( ULONG64 )Fat->Bpb.Dos2_00Bpb.SectorsPerCluster * 512;
            Buffer = ( PUCHAR )Buffer + ( ULONG64 )Fat->Bpb.Dos2_00Bpb.SectorsPerCluster * 512;
            Offset = 0;
        }
        else {

            break;
        }

        ChainIndex++;
    }

    MmFreePoolWithTag( SystemBuffer, FAT_TAG );

    return STATUS_SUCCESS;
}
