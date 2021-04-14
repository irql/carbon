


#include <carbusr.h>
#include "user.h"

NTSTATUS
NtCreateMenu(
    _Out_ PNT_MENU_HANDLE* MenuHandle
)
{
    PNT_MENU_HANDLE Menu;

    //
    // I dont really like this function but I wanted to be consistent
    // with functions like NtCreateFont.
    //

    Menu = RtlAllocateHeap( RtlGetCurrentHeap( ),
                            sizeof( NT_MENU_HANDLE ) );

    Menu->MenuItem = NULL;

    *MenuHandle = Menu;

    return STATUS_SUCCESS;
}

NTSTATUS
NtInsertMenu(
    _In_ PNT_MENU_HANDLE MenuHandle,
    _In_ PWSTR           ParentName,
    _In_ PWSTR           MenuName
)
{
    PMENU_ITEM MenuItem;
    PMENU_ITEM ParentItem;
    PMENU_ITEM NewMenu;

    ParentItem = NULL;

    if ( ParentName != NULL ) {

        if ( MenuHandle->MenuItem == NULL ) {

            return STATUS_INVALID_PARAMETER;
        }

        MenuItem = MenuHandle->MenuItem;

        do {

            if ( wcscmp( MenuItem->Name, ParentName ) == 0 ) {

                ParentItem = MenuItem;
                break;
            }

            MenuItem = MenuItem->Link;
        } while ( MenuItem->Link != NULL );

        if ( ParentItem == NULL ) {

            return STATUS_NOT_FOUND;
        }
    }

    NewMenu = RtlAllocateHeap( RtlGetCurrentHeap( ),
                               sizeof( MENU_ITEM ) );
    NewMenu->Name = MenuName;
    NewMenu->Link = NULL;
    NewMenu->Child = NULL;

    if ( ParentItem == NULL ) {

        if ( MenuHandle->MenuItem == NULL ) {

            MenuHandle->MenuItem = NewMenu;
        }
        else {

            MenuItem = MenuHandle->MenuItem;
            while ( MenuItem->Link != NULL ) {

                MenuItem = MenuItem->Link;
            }

            MenuItem->Link = NewMenu;
        }
    }
    else {

        if ( ParentItem->Child == NULL ) {

            ParentItem->Child = NewMenu;
        }
        else {

            while ( ParentItem->Child != NULL ) {

                ParentItem = ParentItem->Child;
            }

            ParentItem->Child = NewMenu;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NtSetMenu(
    _In_ HANDLE          WindowHandle,
    _In_ PNT_MENU_HANDLE MenuHandle
)
{
    NtSendMessage( WindowHandle, WM_SETMENU, ( ULONG64 )MenuHandle, 0 );
    NtSendMessage( WindowHandle, WM_PAINT, 0, 0 );
    return STATUS_SUCCESS;
}

HANDLE
NtGetContextMenu(

)
{
    //
    // just implement global handles lol?
    //

    HANDLE WindowHandle;
    NtGetWindowByName( &WindowHandle, L"ContextMenu", L"CONTEXT" );
    return WindowHandle;
}
