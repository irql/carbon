


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
    HANDLE ButtonHandle;
    HANDLE StaticHandle;

    WND_CLASS WindowClass;

    KSYSTEM_TIME SystemTime;
    WCHAR BootString[ 512 ];

    LdrInitializeProcess( );

    NtInitializeUser( );

    char data[ 256 ] = { 0 };

    FILE* file;
    file = fopen( "HI.TXT", "r" );
    fseek( file, 5, SEEK_SET );
    if ( file )
        fread( data, 256, 1, file );

    NtQuerySystemClock( &SystemTime );

    fseek( file, 0, SEEK_END );

    swprintf( BootString,
              L"booted on %.2d/%.2d/20%.2d at %.2d:%.2d:%.2d\n"
              L"THE REAL MALLOC: 0x%.8ull\n"
              L"hi.txt reads: %as\n"
              L"size: %d",
              SystemTime.Day,
              SystemTime.Month,
              SystemTime.Year,
              SystemTime.Hour,
              SystemTime.Minute,
              SystemTime.Second,
              malloc( 500 ),
              data,
              ftell( file ) );

    wcscpy( WindowClass.ClassName, L"BITCH" );
    WindowClass.WndProc = NULL;

    NtRegisterClass( &WindowClass );

    //
    // TODO: move most if not all of the classes
    // into user.dll
    //

    NtCreateWindow( &WindowHandle,
                    0,
                    L"window title",
                    L"BITCH",
                    50,
                    50,
                    400,
                    300,
                    0 );
    NtCreateWindow( &ButtonHandle,
                    WindowHandle,
                    L"Brish",
                    L"EDIT",
                    280,
                    24,
                    114,
                    23,
                    0 );
    NtCreateWindow( &StaticHandle,
                    WindowHandle,
                    BootString,
                    L"STATIC",
                    5,
                    24,
                    270,
                    100,
                    0 );

    KUSER_MESSAGE Message;
    HANDLE ContextHandle;
#if 0
    static ULONG32 bits[ ] = {
        0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000,
        0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000,
        0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000,
        0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000,
    };
#endif
    STATIC ULONG32 bits[ 4096 ];
    PNT_FONT_HANDLE FontHandle;

    NTSTATUS ntStatus = NtCreateFont( &FontHandle,
                                      11,
                                      0,
                                      L"MICROSS.TTF" );
    RtlDebugPrint( L"NtCreateFont: %ul\n", ntStatus );
    RECT FontClip;

    FontClip.Left = 0;
    FontClip.Top = 14;
    FontClip.Bottom = 300 - 14;
    FontClip.Right = 400;

    while ( TRUE ) {

        NtWaitMessage( WindowHandle );

        if ( NtReceiveMessage( WindowHandle, &Message ) ) {

            if ( Message.MessageId == WM_PAINT ) {

                NtBeginPaint( &ContextHandle, WindowHandle );

                NtDefaultWindowProc( WindowHandle,
                                     Message.MessageId,
                                     Message.Param1,
                                     Message.Param2 );
#if 0
                NtDrawText( ContextHandle,
                            FontHandle,
                            L"This is some text drawn by user.dll via freetype.dll lol",
                            &FontClip,
                            0,
                            0xFF000000 );
#endif
                NtEndPaint( WindowHandle );

            }
            else {

                NtDefaultWindowProc( WindowHandle,
                                     Message.MessageId,
                                     Message.Param1,
                                     Message.Param2 );
            }
        }

#if 0
        if ( NtReceiveMessage( ButtonHandle, &Message ) ) {
            NtDefaultWindowProc( ButtonHandle,
                                 Message.MessageId,
                                 Message.Param1,
                                 Message.Param2 );
        }
#endif
        if ( NtReceiveMessage( ButtonHandle, &Message ) ) {
            WND_PROC proc;
            NtGetWindowProc( ButtonHandle, &proc );
            proc( ( PKWND )ButtonHandle, Message.MessageId, Message.Param1, Message.Param2 );

        }

        if ( NtReceiveMessage( StaticHandle, &Message ) ) {
            NtDefaultWindowProc( StaticHandle,
                                 Message.MessageId,
                                 Message.Param1,
                                 Message.Param2 );
        }
    }
}
