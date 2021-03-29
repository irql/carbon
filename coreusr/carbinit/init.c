


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
    NtCreateWindow( &EditHandle,
                    WindowHandle,
                    L"\\??\\C:\\SYSTEM",
                    L"EDIT",
                    120 + 5 + 5,
                    24,
                    240,
                    23,
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
    NtCreateWindow( &ButtonHandle,
                    WindowHandle,
                    L"potentially a button",
                    L"BUTTON",
                    120 + 5 + 5,
                    24 + 23 + 5,
                    240,
                    23,
                    0 );

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
    }
}
