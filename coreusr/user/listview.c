


#include <carbusr.h>
#include "user.h"

// add columns of set width, when you care
#if 0
typedef struct _LV_COLUMN {
    ULONG32 Width;
    ULONG32 Count;
    PLV_ITEM

} LV_COLUMN, *PLV_COLUMN;
#endif

typedef struct _LV_DATA {

    ULONG64  SelectedItem;
    ULONG64  ItemCount;
    PLV_ITEM FirstItem;
    ULONG64  LastTickCount;
} LV_DATA, *PLV_DATA;

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
    PLV_ITEM CurrentItem1;
    PLV_DATA ListView;
    WND_INFO Info;
    ULONG32 Width;
    ULONG32 Height;
    LONG64 Scroll;
    ULONG64 PreviousSelected;

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


        ListView = RtlAllocateHeap( NtCurrentPeb( )->ProcessHeap,
                                    sizeof( LV_DATA ) );
        ListView->SelectedItem = ( ULONG64 )-1;
        ListView->FirstItem = NULL;
        ListView->ItemCount = 0;

        NtDefaultWindowProc( WindowHandle,
                             WM_SETUPTR,
                             ( ULONG64 )ListView,
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

        ListView = ( PLV_DATA )NtDefaultWindowProc( WindowHandle,
                                                    WM_GETUPTR,
                                                    0,
                                                    0 );
        CurrentItem = ListView->FirstItem;

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
                            CurrentItem->Id == ListView->SelectedItem ? 0xFFFF0000 : 0xFF000000 );
                FontClip.Left = 4 + 0;
                FontClip.Top += 14;
            }
            CurrentItem = CurrentItem->Link;
        }

        NtEndPaint( WindowHandle );
        break;
    case LV_INSERTITEM:;
        ListView = ( PLV_DATA )NtDefaultWindowProc( WindowHandle,
                                                    WM_GETUPTR,
                                                    0,
                                                    0 );
        CurrentItem = ListView->FirstItem;

        if ( CurrentItem == NULL ) {

            ListView->FirstItem = RtlAllocateHeap( NtCurrentPeb( )->ProcessHeap,
                                                   sizeof( LV_ITEM ) );
            ListView->FirstItem->Id = 0;
            ListView->FirstItem->Name = ( PWSTR )Param1;
            ListView->FirstItem->Link = NULL;
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

        ListView->ItemCount++;
        break;
    case LV_GETITEMCOUNT:;
        ListView = ( PLV_DATA )NtDefaultWindowProc( WindowHandle,
                                                    WM_GETUPTR,
                                                    0,
                                                    0 );
        return ( NTSTATUS )ListView->ItemCount;
    case LV_GETSELECTED:;
        ListView = ( PLV_DATA )NtDefaultWindowProc( WindowHandle,
                                                    WM_GETUPTR,
                                                    0,
                                                    0 );
        return ( NTSTATUS )(
            ListView->SelectedItem == 0 &&
            ListView->ItemCount == 0 ? -1 : ListView->SelectedItem );
    case LV_SETSELECTED:;
        ListView = ( PLV_DATA )NtDefaultWindowProc( WindowHandle,
                                                    WM_GETUPTR,
                                                    0,
                                                    0 );
        ListView->SelectedItem = Param1;
        break;
    case LV_GETITEM:;
        ListView = ( PLV_DATA )NtDefaultWindowProc( WindowHandle,
                                                    WM_GETUPTR,
                                                    0,
                                                    0 );
        CurrentItem = ListView->FirstItem;
        while ( CurrentItem != NULL ) {

            if ( CurrentItem->Id == Param1 ) {

                return ( NTSTATUS )CurrentItem;
            }
            CurrentItem = CurrentItem->Link;
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

        ListView = ( PLV_DATA )NtDefaultWindowProc( WindowHandle,
                                                    WM_GETUPTR,
                                                    0,
                                                    0 );

        if ( ( ULONG64 )Scroll >= ListView->ItemCount ) {

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
    case WM_LMOUSEDOWN:;

        if ( Param2 <= 4 ) {

            break;
        }

        Param2 -= 4;

        Scroll = ( LONG64 )NtDefaultWindowProc( WindowHandle,
                                                WM_GETSCROLLH,
                                                0,
                                                0 );
        ListView = ( PLV_DATA )NtDefaultWindowProc( WindowHandle,
                                                    WM_GETUPTR,
                                                    0,
                                                    0 );

        if ( ( Scroll + Param2 / 14 ) >= ListView->ItemCount ) {

            break;
        }

        PreviousSelected = ListView->SelectedItem;
        ListView->SelectedItem = Scroll + Param2 / 14;

        if ( PreviousSelected != ListView->SelectedItem ) {
            NtSendParentMessage( WindowHandle,
                                 WM_COMMAND,
                                 Info.MenuId,
                                 LV_SELECT );
        }
        else if ( ListView->LastTickCount + 600 >= NtGetTickCount( ) ) {

            NtSendParentMessage( WindowHandle,
                                 WM_COMMAND,
                                 Info.MenuId,
                                 LV_PRESS );
        }

        ListView->LastTickCount = NtGetTickCount( );

        NtSendMessage( WindowHandle,
                       WM_PAINT,
                       Param1,
                       Param2 );
        break;
    case LV_REMOVEITEM:;
        ListView = ( PLV_DATA )NtDefaultWindowProc( WindowHandle,
                                                    WM_GETUPTR,
                                                    0,
                                                    0 );

        CurrentItem = ListView->FirstItem;
        if ( CurrentItem->Id == Param1 ) {

            if ( CurrentItem->Id == ListView->SelectedItem ) {

                ListView->SelectedItem = ( ULONG64 )-1;
            }

            ListView->FirstItem = CurrentItem->Link;
            RtlFreeHeap( NtCurrentPeb( )->ProcessHeap, CurrentItem );
            ListView->ItemCount--;
            break;
        }

        while ( CurrentItem->Link != NULL ) {

            if ( CurrentItem->Link->Id == Param1 ) {

                if ( CurrentItem->Link->Id == ListView->SelectedItem ) {

                    ListView->SelectedItem = ( ULONG64 )-1;
                }

                CurrentItem1 = CurrentItem->Link;
                CurrentItem->Link = CurrentItem->Link->Link;
                RtlFreeHeap( NtCurrentPeb( )->ProcessHeap, CurrentItem1 );
                ListView->ItemCount--;
                return STATUS_SUCCESS;
            }

            CurrentItem = CurrentItem->Link;
        }

        break;
    default:
        return NtDefaultWindowProc( WindowHandle,
                                    MessageId,
                                    Param1,
                                    Param2 );
    }

    return STATUS_SUCCESS;
}
