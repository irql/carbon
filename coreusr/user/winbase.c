


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
    PWB_DATA WindowBaseData;
    PMENU_ITEM MenuItem;
    PMENU_ITEM ChildItem;
    LONG32 LastLeft;
    ULONG32 MenuHeight;

    NtGetWindowInfo( WindowHandle, &Info );

    Width = Info.Rect.Right - Info.Rect.Left;
    Height = Info.Rect.Bottom - Info.Rect.Top;

    FontClip.Left = 5;
    FontClip.Right = Width - 10;
    FontClip.Bottom = 23 - 1 + 8;
    FontClip.Top = 3 + 14;

    switch ( MessageId ) {
    case WM_ACTIVATE:;
        NtCreateFont( &FontHandle,
                      12,
                      0,
                      L"MICROSS.TTF" );

        NtDefaultWindowProc( WindowHandle,
                             WM_SETFONT,
                             ( ULONG64 )FontHandle,
                             0 );
        // hook WM_SETFONT and free this if a program
        // sets the font, don't worry about the address
        // space, only the creator should touch this.

        WindowBaseData = RtlAllocateHeap( RtlGetCurrentHeap( ),
                                          sizeof( WB_DATA ) );
        WindowBaseData->MenuHandle = NULL;

        NtCreateFont( &WindowBaseData->MenuFontHandle,
                      11,
                      0,
                      L"MICROSS.TTF" );

        NtDefaultWindowProc( WindowHandle,
                             WM_SETUPTR,
                             ( ULONG64 )WindowBaseData,
                             0 );

        break;
    case WM_PAINT:;
        NtBeginPaint( &ContextHandle,
                      WindowHandle );

        FontHandle = ( PNT_FONT_HANDLE )NtDefaultWindowProc( WindowHandle,
                                                             WM_GETFONT,
                                                             0,
                                                             0 );
        WindowBaseData = ( PWB_DATA )NtDefaultWindowProc( WindowHandle,
                                                          WM_GETUPTR,
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

        if ( WindowBaseData->MenuHandle != NULL ) {

            FontClip.Left = 6;
            FontClip.Top = 20 + 14;
            FontClip.Bottom = 20 + 14 + 14;
            FontClip.Right = Width - 12;

            if ( WindowBaseData->MenuHandle->MenuItem != NULL ) {

                MenuItem = WindowBaseData->MenuHandle->MenuItem;
                do {
                    LastLeft = FontClip.Left;

                    NtDrawText( ContextHandle,
                                WindowBaseData->MenuFontHandle,
                                MenuItem->Name,
                                &FontClip,
                                0,
                                0xFF000000 );
                    FontClip.Left = LastLeft + WB_MENU_ITEM_WIDTH;

                    MenuItem = MenuItem->Link;
                } while ( MenuItem != NULL );

                MenuItem = WindowBaseData->MenuHandle->MenuItem;
                do {


                    if ( MenuItem->Open ) {
                        FontClip.Top += 14;
                        LastLeft = FontClip.Left;
                        MenuHeight = 0;
                        ChildItem = MenuItem->Child;

                        if ( ChildItem == NULL ) {

                            break;
                        }

                        do {

                            MenuHeight += 14;
                            ChildItem = ChildItem->Child;
                        } while ( ChildItem != NULL );

                        NtClearDC( ContextHandle,
                                   FontClip.Left,
                                   FontClip.Top,
                                   WB_MENU_ITEM_WIDTH,
                                   MenuHeight,
                                   0xFFC3C3C3 );

                        ChildItem = MenuItem->Child;
                        do {
                            NtDrawText( ContextHandle,
                                        WindowBaseData->MenuFontHandle,
                                        MenuItem->Name,
                                        &FontClip,
                                        0,
                                        0xFF000000 );
                            FontClip.Top += 14;
                            FontClip.Left = LastLeft;
                            ChildItem = ChildItem->Child;
                        } while ( ChildItem != NULL );
                        break;
                    }

                    FontClip.Left += WB_MENU_ITEM_WIDTH;
                    MenuItem = MenuItem->Link;
                } while ( MenuItem != NULL );
            }
        }

        NtEndPaint( WindowHandle );
        break;
    case WM_LMOUSEDOWN:

        WindowBaseData = ( PWB_DATA )NtDefaultWindowProc( WindowHandle,
                                                          WM_GETUPTR,
                                                          0,
                                                          0 );

        if ( WindowBaseData->MenuHandle->MenuItem != NULL ) {

            MenuItem = WindowBaseData->MenuHandle->MenuItem;
            do {

                MenuItem->Open = FALSE;
                MenuItem = MenuItem->Link;
            } while ( MenuItem != NULL );
        }

        if ( Param1 >= 6 &&
             Param1 < Width - 12 &&
             Param2 >= 20 &&
             Param2 < 20 + 14 ) {

            Param1 -= 6;

            Param1 /= WB_MENU_ITEM_WIDTH;
            LastLeft = 0;

            WindowBaseData = ( PWB_DATA )NtDefaultWindowProc( WindowHandle,
                                                              WM_GETUPTR,
                                                              0,
                                                              0 );
            if ( WindowBaseData->MenuHandle->MenuItem != NULL ) {

                MenuItem = WindowBaseData->MenuHandle->MenuItem;
                do {

                    if ( Param1 == LastLeft ) {
                        //MenuItem->Open = TRUE;
                        //RtlDebugPrint( L"selected: %s\n", MenuItem->Name );
#if 0
                        HANDLE wnd = NtGetContextMenu( );
                        WND_INFO ctx_info;
                        PNT_MENU_HANDLE MenuHandle1 = ( PNT_MENU_HANDLE )RtlAllocateHeap( RtlGetCurrentHeap( ),
                                                                                          sizeof( NT_MENU_HANDLE ) );

                        NtGetWindowInfo( wnd, &ctx_info );
                        ctx_info.Rect.Right -= ctx_info.Rect.Left;
                        ctx_info.Rect.Bottom -= ctx_info.Rect.Top;
                        ctx_info.Rect.Left = Info.Rect.Left + ( LONG32 )Param1 * WB_MENU_ITEM_WIDTH + 6;
                        ctx_info.Rect.Top = Info.Rect.Top + 20 + 14;
                        ctx_info.Rect.Right += ctx_info.Rect.Left;
                        ctx_info.Rect.Bottom += ctx_info.Rect.Top;
                        NtSetWindowInfo( wnd, &ctx_info );
                        NtSetParent( WindowHandle, wnd );
                        MenuHandle1->MenuItem = MenuItem->Child;
                        NtSetMenu( wnd, MenuHandle1 );
#endif
                        break;
                    }

                    LastLeft++;
                    MenuItem = MenuItem->Link;
                } while ( MenuItem != NULL );
            }
        }
        else {

            return NtDefaultWindowProc( WindowHandle,
                                        MessageId,
                                        Param1,
                                        Param2 );
        }

        break;
    case WM_SETMENU:
        WindowBaseData = ( PWB_DATA )NtDefaultWindowProc( WindowHandle,
                                                          WM_GETUPTR,
                                                          0,
                                                          0 );
        WindowBaseData->MenuHandle = ( PNT_MENU_HANDLE )Param1;
        break;
    default:
        return NtDefaultWindowProc( WindowHandle,
                                    MessageId,
                                    Param1,
                                    Param2 );
    }

    return STATUS_SUCCESS;
}
