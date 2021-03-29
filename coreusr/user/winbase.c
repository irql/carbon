


#define USER_INTERNAL
#include <carbusr.h>
#include "user.h"

NTSTATUS
NtClassWinBaseProc(
    _In_ HANDLE  WindowHandle,
    _In_ ULONG32 MessageId,
    _In_ ULONG64 Param1,
    _In_ ULONG64 Param2
)
{
    Param1;
    Param2;

    HANDLE ContextHandle;
    RECT FontClip;
    ULONG32 Width;
    ULONG32 Height;
    WND_INFO Info;
    PNT_FONT_HANDLE FontHandle;

    NtGetWindowInfo( WindowHandle, &Info );

    Width = Info.Rect.Right - Info.Rect.Left;
    Height = Info.Rect.Bottom - Info.Rect.Top;

    FontClip.Left = 5;
    FontClip.Right = Width - 8;
    FontClip.Bottom = 19 - 3 - 14;
    FontClip.Top = 3 + 14;

    switch ( MessageId ) {
    case WM_ACTIVATE:;
        NtCreateFont( &FontHandle,
                      13,
                      0,
                      L"MICROSS.TTF" );

        NtDefaultWindowProc( WindowHandle,
                             WM_SETFONT,
                             ( ULONG64 )FontHandle,
                             0 );
        // hook WM_SETFONT and free this if a program
        // sets the font, don't worry about the address
        // space, only the creator should touch this.
        break;
    case WM_PAINT:;
        NtBeginPaint( &ContextHandle,
                      WindowHandle );

        FontHandle = ( PNT_FONT_HANDLE )NtDefaultWindowProc( WindowHandle,
                                                             WM_GETFONT,
                                                             0,
                                                             0 );
#if 0
        NtClearDC( ContextHandle, 0, 0, Width, Height, 0xFFC3C3C3 );

        for ( ULONG32 i = 0; i < Width - 2; i++ ) {

            NtSetPixel( ContextHandle, i + 1, 1, 0xFFFFFFFF );
        }

        for ( ULONG32 i = 0; i < Width; i++ ) {

            NtSetPixel( ContextHandle, i, Height - 1, 0xFF000000 );
        }

        for ( ULONG32 i = 0; i < Width - 2; i++ ) {

            NtSetPixel( ContextHandle, i + 1, Height - 2, 0xFF828282 );
            //( ( ULONG32* )Context->DeviceSpecific )[ ( Height - 2 ) * Width + i + 1 ] = 0xFF828282;
        }

        for ( ULONG32 i = 0; i < Height - 2; i++ ) {
            //( ( ULONG32* )Context->DeviceSpecific )[ Width * ( i + 1 ) + 1 ] = 0xFFFFFFFF;
        }

        for ( ULONG32 i = 0; i < Height; i++ ) {
            ( ( ULONG32* )Context->DeviceSpecific )[ Width * ( i + 1 ) - 1 ] = 0xFF000000;
        }

        for ( ULONG32 i = 0; i < Height - 2; i++ ) {
            ( ( ULONG32* )Context->DeviceSpecific )[ Width * ( i + 2 ) - 2 ] = 0xFF828282;
        }

        for ( ULONG32 j = 0; j < 18; j++ ) {
            for ( ULONG32 i = 0; i < Width - 6; i++ ) {
                ( ( ULONG32* )Context->DeviceSpecific )[ Width * 3 + 3 + i + ( j * Width ) ] = Window == FocusWindow ? 0xFF000082 : 0xFF808080;
            }
        }
#endif

        NtDefaultWindowProc( WindowHandle,
                             MessageId,
                             Param1,
                             Param2 );

        NtDrawText( ContextHandle,
                    FontHandle,
                    Info.Name,
                    &FontClip,
                    0,
                    0xFFFFFFFF );

        NtEndPaint( WindowHandle );
        break;
    default:
        break;
    }

    return TRUE;
}
