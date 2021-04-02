


#define USER_INTERNAL
#include <carbusr.h>
#include "user.h"

NTSTATUS
NtClassEditBaseProc(
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
    WCHAR Append[ 2 ] = { 0 };

    NtGetWindowInfo( WindowHandle, &Info );

    Width = Info.Rect.Right - Info.Rect.Left;
    Height = Info.Rect.Bottom - Info.Rect.Top;

    FontClip.Left = 4 + 0;
    FontClip.Right = Width - 8;
    FontClip.Bottom = Height - 2 + 7;
    FontClip.Top = 2 + 14;

    switch ( MessageId ) {
    case WM_ACTIVATE:;
        NtCreateFont( &FontHandle,
                      11,
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

        NtClearDC( ContextHandle, 0, 0, Width, Height, 0xFFFFFFFF );

        for ( ULONG32 i = 0; i < Width - 1; i++ ) {
            NtSetPixel( ContextHandle, i, 0, 0xFF808080 );
        }

        for ( ULONG32 i = 0; i < Height - 1; i++ ) {
            NtSetPixel( ContextHandle, 0, i, 0xFF808080 );
        }

        for ( ULONG32 i = 0; i < Width - 2; i++ ) {
            NtSetPixel( ContextHandle, i + 1, 1, 0xFF000000 );
        }

        for ( ULONG32 i = 0; i < Height - 2; i++ ) {
            NtSetPixel( ContextHandle, 1, i + 1, 0xFF000000 );
        }

        for ( ULONG32 i = 0; i < Height - 2; i++ ) {
            NtSetPixel( ContextHandle, 1, i + 1, 0xFF000000 );
        }

        for ( ULONG32 i = 0; i < Width - 2; i++ ) {
            NtSetPixel( ContextHandle, i + 1, Height - 2, 0xFFC0C0C0 );
        }

        for ( ULONG32 i = 0; i < Height - 2; i++ ) {
            NtSetPixel( ContextHandle, Width - 2, i + 1, 0xFFC0C0C0 );
        }

        FontHandle = ( PNT_FONT_HANDLE )NtDefaultWindowProc( WindowHandle,
                                                             WM_GETFONT,
                                                             0,
                                                             0 );

        NtDrawText( ContextHandle,
                    FontHandle,
                    Info.Name,
                    &FontClip,
                    0,
                    0xFF000000 );

        NtEndPaint( WindowHandle );
        break;
    case WM_KEYDOWN:;

        if ( Param1 == VK_ENTER ) {

            NtSendParentMessage( WindowHandle,
                                 WM_COMMAND,
                                 Info.MenuId,
                                 ED_ENTER );
            break;
        }

        if ( Param1 >= ' ' &&
             Param1 <= '~' ) {

            Append[ 0 ] = ( WCHAR )Param1;
            wcscat( Info.Name, Append );

            NtDefaultWindowProc( WindowHandle,
                                 WM_SETTEXT,
                                 ( ULONG64 )Info.Name,
                                 512 );
        }
        else if ( Param1 == VK_BACK && wcslen( Info.Name ) > 0 ) {

            Info.Name[ wcslen( Info.Name ) - 1 ] = 0;
            NtDefaultWindowProc( WindowHandle,
                                 WM_SETTEXT,
                                 ( ULONG64 )Info.Name,
                                 512 );
        }

        NtSendMessage( WindowHandle,
                       WM_PAINT,
                       0,
                       0 );

        break;
    default:
        return NtDefaultWindowProc( WindowHandle,
                                    MessageId,
                                    Param1,
                                    Param2 );
    }

    return STATUS_SUCCESS;
}
