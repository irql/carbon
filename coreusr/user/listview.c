


#define USER_INTERNAL
#include <carbusr.h>
#include "user.h"

NTSTATUS
NtClassListViewBaseProc(
    _In_ HANDLE  WindowHandle,
    _In_ ULONG32 MessageId,
    _In_ ULONG64 Param1,
    _In_ ULONG64 Param2
)
{
    Param2;

    HANDLE ContextHandle;
    RECT FontClip;
    PNT_FONT_HANDLE FontHandle;
    PLV_ITEM CurrentItem;
    WND_INFO Info;
    ULONG32 Width;
    ULONG32 Height;
    LONG64 Scroll;
    ULONG64 ItemCount;

    NtGetWindowInfo( WindowHandle, &Info );

    Width = Info.Rect.Right - Info.Rect.Left;
    Height = Info.Rect.Bottom - Info.Rect.Top;

    FontClip.Left = 4 + 0;
    FontClip.Right = Width - 8;
    FontClip.Bottom = Height - 4 + 7;
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
        break;
    case WM_PAINT:;
        NtBeginPaint( &ContextHandle,
                      WindowHandle );

        FontHandle = ( PNT_FONT_HANDLE )NtDefaultWindowProc( WindowHandle,
                                                             WM_GETFONT,
                                                             0,
                                                             0 );

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

        CurrentItem = ( PLV_ITEM )NtDefaultWindowProc( WindowHandle,
                                                       WM_GETUPTR,
                                                       0,
                                                       0 );

        Scroll = ( LONG64 )NtDefaultWindowProc( WindowHandle,
                                                WM_GETSCROLLH,
                                                0,
                                                0 );

        while ( CurrentItem != NULL ) {

            if ( CurrentItem->Id >= Scroll ) {

                NtDrawText( ContextHandle,
                            FontHandle,
                            CurrentItem->Name,
                            &FontClip,
                            0,
                            0xFF000000 );
                FontClip.Top += 14;
            }
            CurrentItem = CurrentItem->Link;
        }

        NtEndPaint( WindowHandle );
        break;
    case LV_INSERTITEM:;
        CurrentItem = ( PLV_ITEM )NtDefaultWindowProc( WindowHandle,
                                                       WM_GETUPTR,
                                                       0,
                                                       0 );

        if ( CurrentItem == NULL ) {

            CurrentItem = RtlAllocateHeap( NtCurrentPeb( )->ProcessHeap,
                                           sizeof( LV_ITEM ) );
            CurrentItem->Id = 0;
            CurrentItem->Name = ( PWSTR )Param1;
            CurrentItem->Link = NULL;

            NtDefaultWindowProc( WindowHandle,
                                 WM_SETUPTR,
                                 ( ULONG64 )CurrentItem,
                                 0 );
        }
        else {

            while ( CurrentItem->Link != NULL ) {

                CurrentItem = CurrentItem->Link;
            }

            CurrentItem->Link = RtlAllocateHeap( NtCurrentPeb( )->ProcessHeap,
                                                 sizeof( LV_ITEM ) );
            CurrentItem->Link->Id = CurrentItem->Id + 1;
            CurrentItem->Link->Name = ( PWSTR )Param1;
            CurrentItem->Link->Link = NULL;
        }
        break;
    case WM_VSCROLL:;

        Scroll = ( LONG64 )NtDefaultWindowProc( WindowHandle,
                                                WM_GETSCROLLH,
                                                0,
                                                0 );

        Scroll += ( LONG64 )Param1;

        if ( Scroll < 0 ) {

            break;
        }

        CurrentItem = ( PLV_ITEM )NtDefaultWindowProc( WindowHandle,
                                                       WM_GETUPTR,
                                                       0,
                                                       0 );
        ItemCount = 0;
        while ( CurrentItem != NULL ) {

            ItemCount++;
            CurrentItem = CurrentItem->Link;
        }

        if ( ( ULONG64 )Scroll >= ItemCount ) {

            break;
        }

        NtDefaultWindowProc( WindowHandle,
                             WM_SETSCROLLH,
                             Scroll,
                             Param2 );
        NtSendMessage( WindowHandle,
                       WM_PAINT,
                       Param1,
                       Param2 );
        break;
    default:
        return NtDefaultWindowProc( WindowHandle,
                                    MessageId,
                                    Param1,
                                    Param2 );
    }

    return STATUS_SUCCESS;
}
