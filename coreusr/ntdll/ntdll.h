


#pragma once

NTSTATUS
NtDirectorySplit(
    _In_  PWSTR InputBuffer,
    _Out_ PWSTR ObjectName,
    _Out_ PWSTR RootDirectory
);

//NTSYSAPI void* malloc( size_t size );
//NTSYSAPI void free( void* ptr );
