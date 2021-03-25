


#include <carbusr.h>
#include "../ntdll.h"

FILE* fopen( const char* filename, const char* mode ) {

    FILE* File;
    NTSTATUS ntStatus;
    BOOLEAN Overwrite;
    IO_STATUS_BLOCK StatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes = { 0 };
    PCHAR TempName;

    WCHAR Input[ 256 ];
    RTL_STACK_STRING( ObjectAttributes.ObjectName, 256 );
    RTL_STACK_STRING( ObjectAttributes.RootDirectory, 256 );

    //RtlDebugPrint( L"Fopen: %as %as\n", filename, mode );

    File = malloc( sizeof( FILE ) );
    File->Access = SYNCHRONIZE;
    File->Offset = 0;
    Overwrite = FALSE;

    TempName = strdup( ( char* )filename );

    for ( int i = 0; TempName[ i ]; i++ ) {

        if ( TempName[ i ] == '/' ) {
            TempName[ i ] = '\\';
        }
    }

    for ( int i = 0; mode[ i ]; i++ ) {

        switch ( mode[ i ] ) {
        case 'r':
            File->Access |= GENERIC_READ;
            break;
        case 'w':
            File->Access |= GENERIC_WRITE;
            break;
        case 'a':
            File->AppendMode = TRUE;
            break;
        case '+':
            Overwrite = TRUE;
            break;
        case 'b':
            break;
        default:
            free( File );
            return NULL; // brutal
        }
    }

    //
    // First attempt: we try concatenate the current directory
    // assuming the file name is something like "jon is bussin.txt"
    //

    wcscpy( Input, NtCurrentPeb( )->CurrentDirectory.Buffer );
    mbstowcs( Input + NtCurrentPeb( )->CurrentDirectory.Length / sizeof( WCHAR ),
              TempName,
              ( NtCurrentPeb( )->CurrentDirectory.MaximumLength -
                NtCurrentPeb( )->CurrentDirectory.Length ) / sizeof( WCHAR ) );

    NtDirectorySplit( Input,
                      ObjectAttributes.ObjectName.Buffer,
                      ObjectAttributes.RootDirectory.Buffer );
    ObjectAttributes.ObjectName.Length = ( USHORT )( wcslen( ObjectAttributes.ObjectName.Buffer ) * sizeof( WCHAR ) );
    ObjectAttributes.RootDirectory.Length = ( USHORT )( wcslen( ObjectAttributes.RootDirectory.Buffer ) * sizeof( WCHAR ) );

#if 0
    RtlDebugPrint( L"Sling Em: %s %s\n",
                   ObjectAttributes.ObjectName.Buffer,
                   ObjectAttributes.RootDirectory.Buffer );
#endif

    ntStatus = NtCreateFile( &File->FileHandle,
                             &StatusBlock,
                             File->Access,
                             &ObjectAttributes,
                             Overwrite ? FILE_CREATE : FILE_OPEN_IF,
                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                             0 );

    if ( NT_SUCCESS( ntStatus ) &&
         NT_SUCCESS( StatusBlock.Status ) ) {

        free( TempName );
        return File;
    }

    //
    // Second attempt: we try it as a complete path
    //

    mbstowcs( Input,
              TempName,
              NtCurrentPeb( )->CurrentDirectory.MaximumLength / sizeof( WCHAR ) );

    NtDirectorySplit( Input,
                      ObjectAttributes.ObjectName.Buffer,
                      ObjectAttributes.RootDirectory.Buffer );
    ObjectAttributes.ObjectName.Length = ( USHORT )( wcslen( ObjectAttributes.ObjectName.Buffer ) * sizeof( WCHAR ) );
    ObjectAttributes.RootDirectory.Length = ( USHORT )( wcslen( ObjectAttributes.RootDirectory.Buffer ) * sizeof( WCHAR ) );
#if 0
    RtlDebugPrint( L"Sling Em: %s %s\n",
                   ObjectAttributes.ObjectName.Buffer,
                   ObjectAttributes.RootDirectory.Buffer );
#endif

    ntStatus = NtCreateFile( &File->FileHandle,
                             &StatusBlock,
                             File->Access,
                             &ObjectAttributes,
                             Overwrite ? FILE_CREATE : FILE_OPEN_IF,
                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                             0 );

    if ( NT_SUCCESS( ntStatus ) &&
         NT_SUCCESS( StatusBlock.Status ) ) {

        free( TempName );
        return File;
    }

    free( TempName );
    free( File );
    return NULL;
}

int fclose( FILE* stream ) {

    if ( NT_SUCCESS( NtClose( stream->FileHandle ) ) ) {
        return 0;
    }
    else {
        return EOF;
    }
}

size_t fread( void* ptr, size_t size, size_t count, FILE* stream ) {

    //
    // lol what a fucking stupid prototype
    //

    ULONG64 ByteCount;
    IO_STATUS_BLOCK StatusBlock;
    NTSTATUS ntStatus;

    ByteCount = size * count;
    /*
        RtlDebugPrint( L"Reading %d bytes at offset %d of file (%d)\n",
                       ByteCount,
                       stream->Offset,
                       stream->FileHandle );*/

    if ( ByteCount == 0 ) {

        return 0;
    }

    ntStatus = NtReadFile( stream->FileHandle,
                           0,
                           &StatusBlock,
                           ptr,
                           ByteCount,
                           stream->Offset );
    if ( !NT_SUCCESS( ntStatus ) ||
         !NT_SUCCESS( StatusBlock.Status ) ) {

        return 0;
    }

    stream->Offset += ( long )ByteCount;

    return ByteCount;//StatusBlock.Information;
}

int fseek( FILE* stream, long int offset, int origin ) {
    // "long int"

    NTSTATUS ntStatus;
    FILE_BASIC_INFORMATION Basic;
    IO_STATUS_BLOCK StatusBlock;

    switch ( origin ) {
    case SEEK_SET:
        stream->Offset = offset;
        break;
    case SEEK_CUR:
        stream->Offset += offset;
        break;
    case SEEK_END:

        ntStatus = NtQueryInformationFile( stream->FileHandle,
                                           &StatusBlock,
                                           &Basic,
                                           sizeof( FILE_BASIC_INFORMATION ),
                                           FileBasicInformation );

        if ( !NT_SUCCESS( ntStatus ) ||
             !NT_SUCCESS( StatusBlock.Status ) ) {

            return -1;
        }

        //brutal.
        stream->Offset = ( long )Basic.FileLength;
        break;
    default:
        return -1;
    }
    return 0;
}

long int ftell( FILE* stream ) {

    return stream->Offset;
}
