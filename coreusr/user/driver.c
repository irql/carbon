


#define USER_INTERNAL
#include <carbusr.h>
#include "user.h"

VOID
NtInitializeUser(

)
{


    WND_CLASS TextEdit;
    TextEdit.WndProc = ( WND_PROC )NtClassEditBaseProc;
    wcscpy( TextEdit.ClassName, L"EDIT" );

    NtRegisterClass( &TextEdit );

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
