


#define NTUSER_INTERNAL
#include <carbsup.h>
#include "usersup.h"
#include "ntuser.h"

KSYSTEM_SERVICE NtUserServiceTable[ ] = {
    SYSTEM_SERVICE( NtSendMessage, 0 ),
    SYSTEM_SERVICE( NtReceiveMessage, 0 ),
    SYSTEM_SERVICE( NtCreateWindow, 5 ),
    SYSTEM_SERVICE( NtGetWindowInfo, 0 ),
    SYSTEM_SERVICE( NtSetWindowInfo, 0 ),
    SYSTEM_SERVICE( NtRegisterClass, 0 ),
    SYSTEM_SERVICE( NtUnregisterClass, 0 ),
    SYSTEM_SERVICE( NtDefaultWindowProc, 0 ),
    SYSTEM_SERVICE( NtGetWindowProc, 0 ),
    SYSTEM_SERVICE( NtBeginPaint, 0 ),
    SYSTEM_SERVICE( NtEndPaint, 0 ),
    SYSTEM_SERVICE( NtGetDC, 0 ),
    SYSTEM_SERVICE( NtBlt, 4 ),
    SYSTEM_SERVICE( NtBltBits, 4 ),
    SYSTEM_SERVICE( NtWaitMessage, 0 ),
    SYSTEM_SERVICE( NtSetPixel, 0 ),
    SYSTEM_SERVICE( NtClearDC, 2 ),
};

NTSTATUS
DriverLoad(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    DriverObject;
    WND_CLASS Poggers;
    ULONG32 Width;
    ULONG32 Height;
    ULONG32 BitDepth;

    //
    // Same idea as windows, have elements call into a dll
    // which reads and sends messages to the processes respective
    // handler.
    //
    // dc implementation rundown before sleep: HDC's (handles to device contexts)
    // are going to be specific to dxgi, they should have general apis like GetDC which
    // allow for a user mode program to get the device context for a window or surface, 
    // and then draw their own stuff to it using d3d.dll. also implement a vesafb driver
    // to handle the non-pogger stuff so qemu can run it as well as vmware (but vmware with
    // svga3d accel). The hdcs will represent the client area and are abstracted by dxgi.
    //
    // maybe implement global shared handles like Windows has
    //
    // we need to implement hardware enumeration drivers and implement 
    // apis to have basic os drivers for them, and then allow other drivers
    // to claim these devices and take ownership.
    //
    // Phat issue: windows are sometimes unlinked when handling system messages ?
    //

    KeInstallServiceDescriptorTable( 1,
                                     sizeof( NtUserServiceTable ) / sizeof( KSYSTEM_SERVICE ),
                                     NtUserServiceTable );

    NtInitializeUserFonts( );
    NtInitializeSystemMessages( );
    NtInitializeUserWindows( );

    lstrcpyW( Poggers.ClassName, L"Yeah im pogin" );

    Poggers.DefWndProc = ( WND_PROC )NtClassWindowBaseProc;

    NtRegisterClass( &Poggers );

    NtGetMode( &Width,
               &Height,
               &BitDepth );

    HANDLE WindowHandle;
    NtCreateWindow( &WindowHandle,
                    0,
                    L"Big pogs",
                    L"Yeah im pogin",
                    0,
                    0,
                    Width,
                    Height,
                    0 );
    NtBroadcastDirectMessage( WM_PAINT, 0, 0 );


    return STATUS_SUCCESS;
}
