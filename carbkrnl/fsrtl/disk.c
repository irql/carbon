


#include <carbsup.h>
#include "fsrtlp.h"

#define HARDDISK_DIRECTORY_PARENT   L"\\Device"
#define HARDDISK_DIRECTORY          L"Harddisk%d"
#define HARDDISK_VOLUME_NAME        L"\\Volume%d"
#define HARDDISK_DRIVE_NAME         L"\\Drive"
#define HARDDISK_DRIVE              HARDDISK_DIRECTORY_PARENT L"\\" HARDDISK_DIRECTORY HARDDISK_DRIVE_NAME
#define HARDDISK_VOLUME             HARDDISK_DIRECTORY_PARENT L"\\" HARDDISK_DIRECTORY HARDDISK_VOLUME_NAME

ULONG64 FsRtlpHarddiskNumber = 0;
CHAR    FsRtlpDiskLetter = 'C';

BOOLEAN
FsRtlContainingDirectory(
    _Inout_ PUNICODE_STRING Complete,
    _Inout_ PUNICODE_STRING Directory
)
{
    ULONG64 Index;

    Directory->Length = Complete->Length / sizeof( WCHAR );
    RtlCopyMemory( Directory->Buffer, Complete->Buffer, Directory->Length * sizeof( WCHAR ) + sizeof( WCHAR ) );

    for ( Index = Directory->Length; Index > 0; Index-- ) {

        if ( Directory->Buffer[ Index ] == '\\' ) {

            Directory->Buffer[ Index ] = 0;
            Directory->Length = ( USHORT )RtlStringLength( Directory->Buffer ) * sizeof( WCHAR );
            break;
        }
    }

    return TRUE;
}

USHORT
FsRtlFileNameIndex(
    _In_ PUNICODE_STRING Directory
)
{
    USHORT Index;
    Index = Directory->Length / 2;

    for ( ; Index > 0; Index-- ) {

        if ( Directory->Buffer[ Index ] == '\\' ) {

            return Index + 1;
        }
    }

    return ( USHORT )~( ( USHORT )0ul ); // ok msvc, thank you for warning.
}

BOOLEAN
FsRtlFileName(
    _Inout_ PUNICODE_STRING Complete,
    _Inout_ PUNICODE_STRING Directory
)
{
    USHORT Index;
    Index = Complete->Length;

    Directory->Length = Complete->Length;

    for ( ; Index > 0; Index-- ) {

        if ( Complete->Buffer[ Index ] == '\\' ) {

            Directory->Length = ( Complete->Length - Index );
            RtlCopyMemory( Directory->Buffer, Complete->Buffer + Index, Directory->Length * sizeof( WCHAR ) + sizeof( WCHAR ) );
            break;
        }
    }

    return TRUE;
}

VOID
FsRtlCreateDiskDevice(
    _Out_ PUNICODE_STRING DriveDirectory
)
{ // fix.
    POBJECT_DIRECTORY DeviceDirectory;
    UNICODE_STRING ParentDirectory = RTL_CONSTANT_STRING( HARDDISK_DIRECTORY_PARENT );
    UNICODE_STRING HarddiskDirectory;

    ObReferenceObjectByName( &DeviceDirectory, &ParentDirectory, ObDirectoryObject );

    HarddiskDirectory.Buffer = MmAllocatePoolWithTag( NonPagedPoolZeroed, 256, FSRTL_TAG );
    HarddiskDirectory.Length = 0;
    HarddiskDirectory.MaximumLength = 256;
    RtlFormatBuffer( HarddiskDirectory.Buffer, HARDDISK_DIRECTORY, FsRtlpHarddiskNumber );
    HarddiskDirectory.Length = ( USHORT )( RtlStringLength( HarddiskDirectory.Buffer ) * sizeof( WCHAR ) );

    ObInsertDirectoryEntry( DeviceDirectory, &HarddiskDirectory, ObCreateDirectoryHead( ) );

    DriveDirectory->Buffer = MmAllocatePoolWithTag( NonPagedPoolZeroed, 256, FSRTL_TAG );
    DriveDirectory->Length = 0;
    DriveDirectory->MaximumLength = 256;
    RtlFormatBuffer( DriveDirectory->Buffer, HARDDISK_DRIVE, FsRtlpHarddiskNumber );
    DriveDirectory->Length = ( USHORT )( RtlStringLength( DriveDirectory->Buffer ) * sizeof( WCHAR ) );
    FsRtlpHarddiskNumber++;
}

ULONG64
FsRtlQueryDiskCount(

)
{
    return FsRtlpHarddiskNumber;
}

CHAR
FsRtlNextDriveLetter(

)
{
    return FsRtlpDiskLetter++;
}
