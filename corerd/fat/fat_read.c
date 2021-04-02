


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
    PFAT_DEVICE Fat;

    Fat = FspFatDevice( Request->DeviceObject );
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

    File->Flags = 0;
    if ( Directory[ FatFile ].Short.Attributes & FAT32_DIRECTORY )
        File->Flags |= FILE_FLAG_DIRECTORY;
    if ( Directory[ FatFile ].Short.Attributes & FAT32_HIDDEN )
        File->Flags |= FILE_FLAG_ATTRIBUTE_HIDDEN;
    if ( Directory[ FatFile ].Short.Attributes & FAT32_SYSTEM )
        File->Flags |= FILE_FLAG_ATTRIBUTE_SYSTEM;
    if ( Directory[ FatFile ].Short.Attributes & FAT32_READ_ONLY )
        File->Flags |= FILE_FLAG_ATTRIBUTE_READONLY;

    //if ( File->Flags & FILE_FLAG_DIRECTORY ) {

        //Request->FileObject->FileLength = 512 * Fat->Bpb.Dos2_00Bpb.SectorsPerCluster;
    //}

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

    if ( Decomposed[ 0 ] == NULL ) {

        File->Flags |= FILE_FLAG_DIRECTORY;
        File->Length = 0; // hm
        File->Chain = MmAllocatePoolWithTag( NonPagedPool, sizeof( ULONG32 ), FAT_TAG );
        File->Chain[ 0 ] = Fat->Bpb.Dos7_01Bpb.RootDirectoryCluster;
        File->ChainLength = 1;
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

NTSTATUS
FsQueryIndexFile(
    _In_  PDEVICE_OBJECT              DeviceObject,
    _In_  PFAT_DIRECTORY              Directory,
    _In_  ULONG64                     FileIndex,
    _In_  ULONG64                     Length,
    _Out_ PFILE_DIRECTORY_INFORMATION Information,
    _Out_ ULONG64*                    ReturnLength
)
{
    DeviceObject;

    ULONG64 CurrentIndex;
    ULONG64 CurrentEntry;
    ULONG64 Char;

    ULONG64 EntryShort = ~0ull;
    ULONG64 EntryShortFound = ~0ull;
    ULONG64 EntryLongFound = ~0ull;

    ULONG64 FileNameIndex;

    for ( CurrentEntry = 0, CurrentIndex = 0; Directory[ CurrentEntry ].Short.Name[ 0 ] != 0; CurrentEntry++ ) {

        if ( ( UCHAR )Directory[ CurrentEntry ].Short.Name[ 0 ] == ( UCHAR )FAT32_DIRECTORY_ENTRY_FREE ) {

            continue;
        }

        if ( Directory[ CurrentEntry ].Short.Attributes & FAT32_VOLUME_ID &&
             Directory[ CurrentEntry ].Short.Attributes != FAT32_LFN ) {

            continue;
        }

        if ( Directory[ CurrentEntry ].Short.Attributes == FAT32_LFN &&
             Directory[ CurrentEntry ].Long.OrderOfEntry != 1 ) {

            continue;
        }

        if ( CurrentIndex == FileIndex ) {

            if ( Directory[ CurrentEntry ].Long.Attributes == FAT32_LFN ) {

                EntryShort = CurrentEntry;
                FileNameIndex = 0;
                do {

                    FileNameIndex += 13;

                    if ( Directory[ CurrentEntry ].Long.OrderOfEntry & FAT32_LAST_LFN_ENTRY ) {

                        *ReturnLength = FileNameIndex * sizeof( WCHAR ) + sizeof( WCHAR );
                        break;
                    }
                    CurrentEntry--;

                } while ( CurrentEntry != ( ULONG64 )-1 );

                if ( Length >= sizeof( FILE_DIRECTORY_INFORMATION ) + *ReturnLength ) {

                    CurrentEntry = EntryShort;
                    FileNameIndex = 0;
                    do {

                        lstrncpyW( Information->FileName + FileNameIndex, Directory[ CurrentEntry ].Long.First5Chars, 5 );
                        FileNameIndex += 5;
                        lstrncpyW( Information->FileName + FileNameIndex, Directory[ CurrentEntry ].Long.Next6Chars, 6 );
                        FileNameIndex += 6;
                        lstrncpyW( Information->FileName + FileNameIndex, Directory[ CurrentEntry ].Long.Next2Chars, 2 );
                        FileNameIndex += 2;

                        if ( Directory[ CurrentEntry ].Long.OrderOfEntry & FAT32_LAST_LFN_ENTRY ) {

                            Information->FileName[ FileNameIndex ] = 0;
                            Information->FileNameLength = FileNameIndex;
                            EntryLongFound = CurrentEntry;
                            EntryShortFound = CurrentEntry + 1;
                            break;
                        }
                        CurrentEntry--;

                    } while ( CurrentEntry != ( ULONG64 )-1 );

                    Information->FileAttributes = 0;
                    if ( Directory[ EntryShortFound ].Short.Attributes & FAT32_DIRECTORY )
                        Information->FileAttributes |= FILE_FLAG_DIRECTORY;
                    if ( Directory[ EntryShortFound ].Short.Attributes & FAT32_HIDDEN )
                        Information->FileAttributes |= FILE_FLAG_ATTRIBUTE_HIDDEN;
                    if ( Directory[ EntryShortFound ].Short.Attributes & FAT32_SYSTEM )
                        Information->FileAttributes |= FILE_FLAG_ATTRIBUTE_SYSTEM;
                    if ( Directory[ EntryShortFound ].Short.Attributes & FAT32_READ_ONLY )
                        Information->FileAttributes |= FILE_FLAG_ATTRIBUTE_READONLY;

                    //EntryLongFound
                    //EntryShortFound

                    return STATUS_SUCCESS;
                }
                else {

                    *ReturnLength += sizeof( FILE_DIRECTORY_INFORMATION );
                    return STATUS_BUFFER_TOO_SMALL;
                }
            }
            else {

                if ( Length >= sizeof( FILE_DIRECTORY_INFORMATION ) + sizeof( WCHAR ) * 12 ) {

                    Information->FileNameLength = 11;

                    for ( Char = 0; Char < 8; Char++ ) {

                        Information->FileName[ Char ] = Directory[ CurrentEntry ].Short.Name[ Char ];
                    }
                    Information->FileName[ 8 ] = 0;

                    for ( Char = 7; Char != 0; Char-- ) {

                        if ( Information->FileName[ Char ] == ' ' ) {

                            Information->FileName[ Char ] = 0;
                        }
                        else {

                            break;
                        }
                    }

                    // we do a little trolling
                    if ( ( Directory[ CurrentEntry ].Short.Attributes & FAT32_DIRECTORY ) == 0 &&
                         *( ULONG32* )Directory[ CurrentEntry ].Short.Extension >> 8 != 0x202020 ) {

                        Length = ++Char;
                        *( ULONG32* )( Information->FileName + Length++ ) = '.';
                        for ( Char = 0; Char < 3; Char++ ) {

                            Information->FileName[ Length++ ] =
                                Directory[ CurrentEntry ].Short.Extension[ Char ];
                        }
                        Information->FileName[ Length ] = 0;
                    }

                    Information->FileAttributes = 0;
                    if ( Directory[ CurrentEntry ].Short.Attributes & FAT32_DIRECTORY )
                        Information->FileAttributes |= FILE_FLAG_DIRECTORY;
                    if ( Directory[ CurrentEntry ].Short.Attributes & FAT32_HIDDEN )
                        Information->FileAttributes |= FILE_FLAG_ATTRIBUTE_HIDDEN;
                    if ( Directory[ CurrentEntry ].Short.Attributes & FAT32_SYSTEM )
                        Information->FileAttributes |= FILE_FLAG_ATTRIBUTE_SYSTEM;
                    if ( Directory[ CurrentEntry ].Short.Attributes & FAT32_READ_ONLY )
                        Information->FileAttributes |= FILE_FLAG_ATTRIBUTE_READONLY;
                }
                else {

                    *ReturnLength = sizeof( FILE_DIRECTORY_INFORMATION ) + sizeof( WCHAR ) * 12;
                    return STATUS_BUFFER_TOO_SMALL;
                }
            }

            return STATUS_SUCCESS;
        }

        CurrentIndex++;
    }

    return STATUS_NO_MORE_FILES;
}

NTSTATUS
FsQueryNameFile(
    _In_  PDEVICE_OBJECT              DeviceObject,
    _In_  PFAT_DIRECTORY              Directory,
    _In_  PUNICODE_STRING             FileName,
    _In_  ULONG64                     Length,
    _Out_ PFILE_DIRECTORY_INFORMATION Information,
    _Out_ ULONG64*                    ReturnLength
)
{
    FAT_PATH_TYPE Type;
    CHAR FileName8Dot3[ 12 ];
    PWCHAR FileNameLfn;
    ULONG64 FatFile;
    PFAT_DEVICE Fat;

    *ReturnLength = sizeof( FILE_DIRECTORY_INFORMATION ) + FileName->Length + sizeof( WCHAR );

    if ( Length < *ReturnLength ) {

        return STATUS_BUFFER_TOO_SMALL;
    }

    Fat = FspFatDevice( DeviceObject );
    Type = FspValidateFileName( FileName->Buffer );

    if ( Type == Path8Dot3 ) {

        FspConvertPathTo8Dot3( FileName->Buffer, FileName8Dot3 );

        FatFile = FspFindDirectoryFile( Directory, Type, FileName8Dot3 );
    }
    else if ( Type == PathLongFileName ) {

        FileNameLfn = FileName->Buffer;
        FatFile = FspFindDirectoryFile( Directory, Type, FileNameLfn );
    }
    else {

        return STATUS_INVALID_PATH;
    }

    if ( FatFile == ( ULONG64 )-1 ) {

        return STATUS_NOT_FOUND;
    }

    Information->FileAttributes = 0;
    if ( Directory[ FatFile ].Short.Attributes & FAT32_DIRECTORY )
        Information->FileAttributes |= FILE_FLAG_DIRECTORY;
    if ( Directory[ FatFile ].Short.Attributes & FAT32_HIDDEN )
        Information->FileAttributes |= FILE_FLAG_ATTRIBUTE_HIDDEN;
    if ( Directory[ FatFile ].Short.Attributes & FAT32_SYSTEM )
        Information->FileAttributes |= FILE_FLAG_ATTRIBUTE_SYSTEM;
    if ( Directory[ FatFile ].Short.Attributes & FAT32_READ_ONLY )
        Information->FileAttributes |= FILE_FLAG_ATTRIBUTE_READONLY;
    lstrcpyW( Information->FileName, FileName->Buffer );

    return STATUS_SUCCESS;
}
