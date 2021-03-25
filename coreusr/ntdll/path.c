


#include <carbusr.h>
#include "ntdll.h"

NTSTATUS
NtDirectorySplit(
    _In_  PWSTR InputBuffer,
    _Out_ PWSTR ObjectName,
    _Out_ PWSTR RootDirectory
)
{
    // TODO: fix this shit up

    //
    // -> \??\
    // -> C:\root
    // -> [global name]\[root]..

    ULONG64 ObjectIndex;
    ULONG64 RootIndex;
    WCHAR TempBuffer[ 256 ];

    ObjectIndex = 0;
    RootIndex = 0;

    for ( ObjectIndex = 0; InputBuffer[ ObjectIndex ] != 0; ObjectIndex++ ) {

        if ( InputBuffer[ ObjectIndex ] == '\\' ) {

            break;
        }

        TempBuffer[ ObjectIndex ] = InputBuffer[ ObjectIndex ];
    }

    wcscpy( ObjectName, L"\\??\\" );
    wcscat( ObjectName, TempBuffer );
    ObjectName[ ObjectIndex + 4 ] = 0;

    for ( RootIndex = 0; InputBuffer[ ObjectIndex + RootIndex ] != 0; RootIndex++ ) {

        RootDirectory[ RootIndex ] = InputBuffer[ ObjectIndex + RootIndex ];
    }
    RootDirectory[ RootIndex ] = 0;

    return STATUS_SUCCESS;
}
