


#include <carbusr.h>
#include "../../ddi/d3d/d3d.h"

#include <ft2build.h>
#include FT_FREETYPE_H

int _fltused = 0;

FT_Library FreeTypeLibrary;

VOID
NtProcessStartup(

)
{
    HANDLE WindowHandle;
    HANDLE EditHandle;
    HANDLE StaticHandle;
    HANDLE ButtonHandle;
    HANDLE ListViewHandle;

    WND_CLASS WindowClass;

    WND_PROC WndProc;

    LdrInitializeProcess( );

    NtInitializeUser( );

    wcscpy( WindowClass.ClassName, L"BITCH" );
    WindowClass.WndProc = NtClassWinBaseProc;

    NtRegisterClass( &WindowClass );

    //
    // TODO: move most if not all of the classes
    // into user.dll
    //

    NtCreateWindow( &WindowHandle,
                    0,
                    L"L4X-Gaming-Coding-Editing",
                    L"BITCH",
                    50,
                    50,
                    400,
                    300,
                    0 );

    NtCreateWindow( &StaticHandle,
                    WindowHandle,
                    L"File name:",
                    L"STATIC",
                    5,
                    24,
                    120,
                    24,
                    0 );
    NtCreateWindow( &EditHandle,
                    WindowHandle,
                    L"\\??\\C:\\SYSTEM",
                    L"EDIT",
                    120 + 5 + 5,
                    24,
                    240,
                    23,
                    0 );


    NtCreateWindow( &ButtonHandle,
                    WindowHandle,
                    L"potentially a button",
                    L"BUTTON",
                    120 + 5 + 5,
                    24 + 23 + 5,
                    240,
                    23,
                    0 );
#if 0
    NtCreateWindow( &ListViewHandle,
                    WindowHandle,
                    L"",
                    L"LISTVIEW",
                    5,
                    24 + 23 + 5,
                    120,
                    99,
                    0 );
#endif

    NtCreateWindow( &ListViewHandle,
                    WindowHandle,
                    L"",
                    L"LISTVIEW",
                    120 + 5 + 5,
                    24 + 23 + 5 + 23 + 5,
                    240,
                    120,
                    0 );

    NtGetWindowProc( ListViewHandle, &WndProc );

    CHAR Buffer[ 256 ];
    PFILE_DIRECTORY_INFORMATION Directory = ( PFILE_DIRECTORY_INFORMATION )&Buffer;
    NTSTATUS ntStatus;

    FILE* filss = fopen( "C:\\SYSTEM\\", "r" );
    IO_STATUS_BLOCK StatusBlock;
    ULONG64 FileIndex = 0;

    do {

        ntStatus = NtQueryDirectoryFile( filss->FileHandle,
                                         &StatusBlock,
                                         Directory,
                                         256,
                                         FileDirectoryInformation,
                                         NULL,
                                         FileIndex,
                                         TRUE );

        if ( !NT_SUCCESS( ntStatus ) || !NT_SUCCESS( StatusBlock.Status ) ) {

            RtlDebugPrint( L"Err: %" );
            break;
        }

        RtlDebugPrint( L"File %s %d\n", Directory->FileName, FileIndex );
        FileIndex++;

        WndProc( ListViewHandle,
                 LV_INSERTITEM,
                 ( ULONG64 )wcsdup( Directory->FileName ),
                 0 );
    } while ( TRUE );

    KUSER_MESSAGE Message;

    while ( TRUE ) {

        NtWaitMessage( WindowHandle );

        if ( NtReceiveMessage( WindowHandle, &Message ) ) {

            NtGetWindowProc( WindowHandle, &WndProc );
            WndProc( WindowHandle, Message.MessageId, Message.Param1, Message.Param2 );
        }

        if ( NtReceiveMessage( EditHandle, &Message ) ) {

            NtGetWindowProc( EditHandle, &WndProc );
            WndProc( EditHandle, Message.MessageId, Message.Param1, Message.Param2 );
        }

        if ( NtReceiveMessage( StaticHandle, &Message ) ) {

            NtGetWindowProc( StaticHandle, &WndProc );
            WndProc( StaticHandle, Message.MessageId, Message.Param1, Message.Param2 );
        }

        if ( NtReceiveMessage( ButtonHandle, &Message ) ) {

            NtGetWindowProc( ButtonHandle, &WndProc );
            WndProc( ButtonHandle, Message.MessageId, Message.Param1, Message.Param2 );
        }

        if ( NtReceiveMessage( ListViewHandle, &Message ) ) {

            NtGetWindowProc( ListViewHandle, &WndProc );
            WndProc( ListViewHandle, Message.MessageId, Message.Param1, Message.Param2 );
        }
    }
}
