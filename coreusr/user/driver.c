


#include <carbusr.h>
#include "user.h"

VOID
NtInitializeUser(

)
{
    WND_CLASS EDIT_Class;
    WND_CLASS STATIC_Class;
    WND_CLASS BUTTON_Class;
    WND_CLASS LISTVIEW_Class;

    EDIT_Class.WndProc = ( WND_PROC )NtClassEditBaseProc;
    wcscpy( EDIT_Class.ClassName, L"EDIT" );

    STATIC_Class.WndProc = ( WND_PROC )NtClassStaticBaseProc;
    wcscpy( STATIC_Class.ClassName, L"STATIC" );

    BUTTON_Class.WndProc = ( WND_PROC )NtClassButtonBaseProc;
    wcscpy( BUTTON_Class.ClassName, L"BUTTON" );

    LISTVIEW_Class.WndProc = ( WND_PROC )NtClassListViewBaseProc;
    wcscpy( LISTVIEW_Class.ClassName, L"LISTVIEW" );

    NtRegisterClass( &EDIT_Class );
    NtRegisterClass( &STATIC_Class );
    NtRegisterClass( &BUTTON_Class );
    NtRegisterClass( &LISTVIEW_Class );
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
