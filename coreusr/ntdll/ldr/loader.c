


#include <carbusr.h>
#include "ldrp.h"
#include "../ntdll.h"

//
// !!! 
// IMPORTANT TODO: Implement proper path parsing for user mode
//                 this function is currently hardcoding \??\BootDevice
//                 because there are no proper apis to decode a normal path
//                 like C:\SYSTEM\DXGIU.DLL into it's counter-parts.
//

NTSTATUS
LdrLoadDll(
    _In_  PWSTR  DirectoryFile,
    _Out_ PVOID* ModuleHandle
)
{
    NTSTATUS ntStatus;
    HANDLE SectionHandle;
    HANDLE FileHandle;
    OBJECT_ATTRIBUTES File = { 0 };
    OBJECT_ATTRIBUTES oa = { 0 };
    IO_STATUS_BLOCK iosb = { 0 };
    PIMAGE_DOS_HEADER DosHeader;
    PIMAGE_NT_HEADERS NtHeaders;
    PLDR_DLL_ENTRY_POINT EntryPoint;

    WCHAR ObjectName[ 256 ];
    WCHAR RootDirectory[ 256 ];

    FileHandle = 0;
    SectionHandle = 0;

    NtDirectorySplit( DirectoryFile,
                      ObjectName,
                      RootDirectory );

    File.RootDirectory.Buffer = RootDirectory;
    File.RootDirectory.Length = ( USHORT )wcslen( RootDirectory ) * sizeof( WCHAR );
    File.RootDirectory.MaximumLength = File.RootDirectory.Length + sizeof( WCHAR );
    File.ObjectName.Buffer = ObjectName;
    File.ObjectName.Length = ( USHORT )wcslen( ObjectName ) * sizeof( WCHAR );
    File.ObjectName.MaximumLength = File.ObjectName.Length + sizeof( WCHAR );

    ntStatus = NtCreateFile( &FileHandle,
                             &iosb,
                             GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | SYNCHRONIZE,
                             &File,
                             FILE_OPEN_IF,
                             FILE_SHARE_READ,
                             0 );
    if ( !NT_SUCCESS( ntStatus ) ||
         !NT_SUCCESS( iosb.Status ) ) {

        goto LdrpProcedureFinished;
    }


    ntStatus = NtCreateSection( &SectionHandle,
                                SECTION_ALL_ACCESS,
                                &oa,
                                SEC_EXECUTE | SEC_WRITE | SEC_IMAGE,
                                FileHandle );
    if ( !NT_SUCCESS( ntStatus ) ) {

        goto LdrpProcedureFinished;
    }

    ntStatus = NtMapViewOfSection( SectionHandle,
                                   NtCurrentProcess( ),
                                   ModuleHandle,
                                   0,
                                   0,
                                   PAGE_READWRITE_EXECUTE );
    if ( !NT_SUCCESS( ntStatus ) ) {

        goto LdrpProcedureFinished;
    }

    DosHeader = ( PIMAGE_DOS_HEADER )( *ModuleHandle );
    NtHeaders = ( PIMAGE_NT_HEADERS )( ( ULONG64 )*ModuleHandle + DosHeader->e_lfanew );
    EntryPoint = ( PLDR_DLL_ENTRY_POINT )( ( ULONG64 )*ModuleHandle +
                                           NtHeaders->OptionalHeader.AddressOfEntryPoint );

    EntryPoint( *ModuleHandle, 0 );

LdrpProcedureFinished:;

    if ( FileHandle != 0 ) {

        NtClose( FileHandle );
    }

    if ( SectionHandle != 0 ) {

        NtClose( SectionHandle );
    }

    return ntStatus;
}
