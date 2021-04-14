


#include <carbusr.h>
#include "user.h"

NTSTATUS
NtContextMenuWndProc(
    _In_ HANDLE  WindowHandle,
    _In_ ULONG32 MessageId,
    _In_ ULONG64 Param1,
    _In_ ULONG64 Param2
)
{
    HANDLE ContextHandle;
    RECT FontClip;
    PWB_DATA WindowBaseData;
    ULONG32 Width;
    ULONG32 Height;
    WND_INFO Info;
    PMENU_ITEM MenuItem;
    ULONG64 CurrentItem;
    ULONG32 MenuHeight;

    NtGetWindowInfo( WindowHandle, &Info );

    Width = Info.Rect.Right - Info.Rect.Left;
    Height = Info.Rect.Bottom - Info.Rect.Top;

    FontClip.Left = 5;
    FontClip.Right = Width - 10;
    FontClip.Top = 3 + 14;
    FontClip.Bottom = 100 + 14;

    switch ( MessageId ) {
    case WM_ACTIVATE:

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
    case WM_PAINT:

        NtBeginPaint( &ContextHandle,
                      WindowHandle );

        WindowBaseData = ( PWB_DATA )NtDefaultWindowProc( WindowHandle,
                                                          WM_GETUPTR,
                                                          0,
                                                          0 );
        if ( WindowBaseData->MenuHandle != NULL ) {
            if ( WindowBaseData->MenuHandle->MenuItem != NULL ) {
                MenuHeight = 0;
                MenuItem = WindowBaseData->MenuHandle->MenuItem;
                do {
                    MenuHeight += WB_MENU_ITEM_HEIGHT;
                    MenuItem = MenuItem->Child;
                } while ( MenuItem != NULL );

                NtClearDC( ContextHandle,
                           0,
                           0,
                           WB_MENU_ITEM_WIDTH,
                           MenuHeight,
                           0xFFFF0000 );

                MenuItem = WindowBaseData->MenuHandle->MenuItem;
                do {


                    NtDrawText( ContextHandle,
                                WindowBaseData->MenuFontHandle,
                                MenuItem->Name,
                                &FontClip,
                                0,
                                0xFF000000 );

                    FontClip.Left = 5;
                    FontClip.Top += WB_MENU_ITEM_HEIGHT;
                    MenuItem = MenuItem->Child;
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

        if ( WindowBaseData->MenuHandle == NULL ) {

            break;
        }

        if ( WindowBaseData->MenuHandle->MenuItem != NULL ) {

            MenuItem = WindowBaseData->MenuHandle->MenuItem;
            do {

                MenuItem->Open = FALSE;
                MenuItem = MenuItem->Link;
            } while ( MenuItem != NULL );
        }

        Param2 /= WB_MENU_ITEM_HEIGHT;
        CurrentItem = 0;
        WindowBaseData = ( PWB_DATA )NtDefaultWindowProc( WindowHandle,
                                                          WM_GETUPTR,
                                                          0,
                                                          0 );
        if ( WindowBaseData->MenuHandle->MenuItem != NULL ) {

            MenuItem = WindowBaseData->MenuHandle->MenuItem;
            do {

                if ( Param2 == CurrentItem ) {

                    if ( MenuItem->Child == NULL ) {

                        NtSendParentMessage( WindowHandle,
                                             WM_COMMAND,
                                             MenuItem->MenuId,
                                             0 );
                    }
                    else {

                        MenuItem->Open = TRUE;
                    }
                    break;
                }

                CurrentItem++;
                MenuItem = MenuItem->Link;
            } while ( MenuItem != NULL );
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

VOID
UxPumpContext(
    _In_ HANDLE ContextMenu
)
{
    KUSER_MESSAGE Message;
    WND_PROC WndProc;

    while ( TRUE ) {

        NtWaitMessage( ContextMenu );

        if ( NtReceiveMessage( ContextMenu, &Message ) ) {

            NtGetWindowProc( ContextMenu, &WndProc );

            WndProc( ContextMenu,
                     Message.MessageId,
                     Message.Param1,
                     Message.Param2 );
        }
    }

}

VOID
NtInitializeUser(

)
{
    WND_CLASS EDIT_Class;
    WND_CLASS STATIC_Class;
    WND_CLASS BUTTON_Class;
    WND_CLASS LISTVIEW_Class;
    WND_CLASS CONTEXT_Class;
    HANDLE ContextMenu;
    HANDLE ThreadHandle;
    OBJECT_ATTRIBUTES Thread = { 0 };

    EDIT_Class.WndProc = ( WND_PROC )NtClassEditBaseProc;
    wcscpy( EDIT_Class.ClassName, L"EDIT" );

    STATIC_Class.WndProc = ( WND_PROC )NtClassStaticBaseProc;
    wcscpy( STATIC_Class.ClassName, L"STATIC" );

    BUTTON_Class.WndProc = ( WND_PROC )NtClassButtonBaseProc;
    wcscpy( BUTTON_Class.ClassName, L"BUTTON" );

    LISTVIEW_Class.WndProc = ( WND_PROC )NtClassListViewBaseProc;
    wcscpy( LISTVIEW_Class.ClassName, L"LISTVIEW" );

    CONTEXT_Class.WndProc = ( WND_PROC )NtContextMenuWndProc;
    wcscpy( CONTEXT_Class.ClassName, L"CONTEXT" );

    NtRegisterClass( &EDIT_Class );
    NtRegisterClass( &STATIC_Class );
    NtRegisterClass( &BUTTON_Class );
    NtRegisterClass( &LISTVIEW_Class );
    NtRegisterClass( &CONTEXT_Class );

    NtCreateWindow( &ContextMenu,
                    INVALID_HANDLE_VALUE,
                    L"ContextMenu",
                    L"CONTEXT",
                    0,
                    0,
                    WB_MENU_ITEM_WIDTH,
                    100,
                    0 );

    NtCreateThread( &ThreadHandle,
                    NtCurrentProcess( ),
                    THREAD_ALL_ACCESS,
                    ( PKSTART_ROUTINE )UxPumpContext,
                    ( PVOID )ContextMenu,
                    0,
                    &Thread,
                    0,
                    NULL );

}

VOID
NtDllLoad(
    _In_ PVOID ModuleBase,
    _In_ ULONG CallReason
)
{
    ModuleBase;
    CallReason;



    return;
}
