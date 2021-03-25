


#pragma once

NTSYSAPI
VOID
FsRtlCreateDiskDevice(
    _In_ PUNICODE_STRING DriveDirectory
);

NTSYSAPI
ULONG64
FsRtlQueryDiskCount(

);

NTSYSAPI
BOOLEAN
FsRtlContainingDirectory(
    _Inout_ PUNICODE_STRING Complete,
    _Inout_ PUNICODE_STRING Directory
);

NTSYSAPI
BOOLEAN
FsRtlFileName(
    _Inout_ PUNICODE_STRING Complete,
    _Inout_ PUNICODE_STRING Directory
);

NTSYSAPI
CHAR
FsRtlNextDriveLetter(

);

NTSYSAPI
USHORT
FsRtlFileNameIndex(
    _In_ PUNICODE_STRING Directory
);