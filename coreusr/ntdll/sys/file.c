


#include <carbusr.h>
#include "../ntdll.h"

HANDLE
CreateFileW(
    _In_ PCWSTR FileName,
    _In_ ULONG  DesiredAccess,
    _In_ ULONG  ShareAccess,
    _In_ ULONG  Disposition,
    _In_ ULONG  Flags
)
{
    Flags;
    NTSTATUS ntStatus;
    OBJECT_ATTRIBUTES File = { 0 };
    IO_STATUS_BLOCK StatusBlock;
    HANDLE FileHandle;
    WCHAR Buffer[ 256 ];

    RTL_STACK_STRING( File.ObjectName, 256 );
    RTL_STACK_STRING( File.RootDirectory, 256 );

    DesiredAccess |= SYNCHRONIZE;

    wcscpy( Buffer, NtCurrentPeb( )->CurrentDirectory.Buffer );
    wcscat( Buffer, FileName );
    NtDirectorySplit( Buffer,
                      File.ObjectName.Buffer,
                      File.RootDirectory.Buffer );
    File.ObjectName.Length = ( USHORT )wcslen( File.ObjectName.Buffer ) * sizeof( WCHAR );
    File.RootDirectory.Length = ( USHORT )wcslen( File.RootDirectory.Buffer ) * sizeof( WCHAR );

    ntStatus = NtCreateFile( &FileHandle,
                             &StatusBlock,
                             DesiredAccess,
                             &File,
                             Disposition,
                             ShareAccess,
                             0 );
    if ( NT_SUCCESS( ntStatus ) &&
         NT_SUCCESS( StatusBlock.Status ) ) {

        return FileHandle;
    }

    NtDirectorySplit( ( PWSTR )FileName,
                      File.ObjectName.Buffer,
                      File.RootDirectory.Buffer );
    File.ObjectName.Length = ( USHORT )wcslen( File.ObjectName.Buffer ) * sizeof( WCHAR );
    File.RootDirectory.Length = ( USHORT )wcslen( File.RootDirectory.Buffer ) * sizeof( WCHAR );

    ntStatus = NtCreateFile( &FileHandle,
                             &StatusBlock,
                             DesiredAccess,
                             &File,
                             Disposition,
                             ShareAccess,
                             0 );
    if ( NT_SUCCESS( ntStatus ) &&
         NT_SUCCESS( StatusBlock.Status ) ) {

        return FileHandle;
    }

    return INVALID_HANDLE_VALUE;
}

BOOL
SetCurrentDirectoryW(
    _In_ PCWSTR PathName
)
{
    //verify.

    wcscpy( NtCurrentPeb( )->CurrentDirectory.Buffer, PathName );

    return TRUE;
}

VOID
GetCurrentDirectoryW(
    _In_ PWSTR PathName,
    _In_ ULONG Length
)
{
    wcsncpy( PathName, NtCurrentPeb( )->CurrentDirectory.Buffer, Length );
}
