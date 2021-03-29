


#define NTUSER_INTERNAL
#include <carbsup.h>
#include "../usersup.h"
#include "../ntuser.h"

NTSTATUS
NtClassWindowBaseProc(
    _In_ PKWND   Window,
    _In_ ULONG32 MessageId,
    _In_ ULONG64 Param1,
    _In_ ULONG64 Param2
)
{
    Param1;
    Param2;

    //
    // I think the user mode programs will call DefWindowProc or similar
    // and that will syscall into this.
    //
    // user mode portion will call into NtReceiveMessage and then call 
    // its own wndproc, (could be completed by user.dll) and then  the
    // wndproc will call DefWindowProc which will call into this.
    //

    PDC Context;
    ULONG32 Width;
    ULONG32 Height;
    BOOLEAN PaintAlreadyBegan;

    Width = Window->FrontContext->ClientArea.Right - Window->FrontContext->ClientArea.Left;
    Height = Window->FrontContext->ClientArea.Bottom - Window->FrontContext->ClientArea.Top;

    switch ( MessageId ) {
    case WM_PAINT:;

        //RtlDebugPrint( L"painted!" );

        //
        // Decided to only use a 3d hardware blitter, instead of optimizing everything
        // with 3d accel, plus this is kernel mode and there is not much of a graphical
        // interface.
        //

        //
        // Win 9x borders : - ], make a func
        //

        //
        // if the user mode proc has began painting on its own,
        // then we dont want to update the screen just yet and commit our changes
        //

        PaintAlreadyBegan = Window->PaintBegan;

        NtDdiBeginPaint( &Context,
                         Window );

        if ( Window == RootWindow ) {

            for ( ULONG32 i = 0; i < Width * Height; i++ ) {
                ( ( ULONG32* )Context->DeviceSpecific )[ i ] = 0xFF008080;
            }
        }
        else {

            for ( ULONG32 i = 0; i < Width * Height; i++ ) {
                ( ( ULONG32* )Context->DeviceSpecific )[ i ] = 0xFFC3C3C3;
            }

            for ( ULONG32 i = 0; i < Width - 2; i++ ) {
                ( ( ULONG32* )Context->DeviceSpecific )[ Width + i + 1 ] = 0xFFFFFFFF;
            }

            for ( ULONG32 i = 0; i < Width; i++ ) {
                ( ( ULONG32* )Context->DeviceSpecific )[ ( Height - 1 ) * Width + i ] = 0xFF000000;
            }

            for ( ULONG32 i = 0; i < Width - 2; i++ ) {
                ( ( ULONG32* )Context->DeviceSpecific )[ ( Height - 2 ) * Width + i + 1 ] = 0xFF828282;
            }

            for ( ULONG32 i = 0; i < Height - 2; i++ ) {
                ( ( ULONG32* )Context->DeviceSpecific )[ Width * ( i + 1 ) + 1 ] = 0xFFFFFFFF;
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
#if 0
            RECT Clip;

            Clip.Left = 5;
            Clip.Top = 4;
            Clip.Right = Width - 8;
            Clip.Bottom = 19;

            NtSystemFont->Engine->Render( NtSystemFont,
                                          Context,
                                          Window->WindowInfo.Name,
                                          &Clip,
                                          0xFFFFFFFF );
#endif
        }

        if ( !PaintAlreadyBegan ) {

            NtDdiEndPaint( Window );
        }

        break;
    case WM_SETFONT:;
        Window->Font = ( PVOID )Param1;

        break;
    case WM_GETFONT:;

        return ( NTSTATUS )Window->Font;
    case WM_GETTEXT:

        wcsncpy( ( wchar_t* )Param1, Window->WindowInfo.Name, Param2 );
        Window->WindowInfo.Name[ 511 ] = 0;
        break;
    case WM_SETTEXT:

        wcsncpy( Window->WindowInfo.Name, ( wchar_t* )Param1, Param2 );
        Window->WindowInfo.Name[ 511 ] = 0;
        break;
    default:
        break;
    }

    return STATUS_SUCCESS;
}
