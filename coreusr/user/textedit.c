


#define USER_INTERNAL
#include <carbusr.h>
#include "user.h"

BOOLEAN
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
    STATIC PNT_FONT_HANDLE FontHandle;
    RECT FontClip;
    ULONG32 Width;
    ULONG32 Height;
    WND_INFO Info;

    NtGetWindowInfo( WindowHandle, &Info );

    Width = Info.Rect.Right - Info.Rect.Left;
    Height = Info.Rect.Bottom - Info.Rect.Top;

    FontClip.Left = 4 + 0;
    FontClip.Right = Width;
    FontClip.Bottom = Height;
    FontClip.Top = 2 + 14;

    switch ( MessageId ) {
    case WM_ACTIVATE:;
        NTSTATUS nt = NtCreateFont( &FontHandle,
                                    11,
                                    0,
                                    L"MICROSS.TTF" );
        if ( !NT_SUCCESS( nt ) ) {

            RtlDebugPrint( L"Brual %ul\n", nt );
        }
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
#if 0
        nt = NtDrawText( ContextHandle,
                         FontHandle,
                         L"Pogging is wrong.",
                         &FontClip,
                         0,
                         0xFF000000 );
#endif
        NtEndPaint( WindowHandle );
        break;
    case WM_KEYDOWN:;

        //RtlDebugPrint( L"key down %ac\n", Param1 );
        break;
    default:
        break;
    }

    return TRUE;
}
