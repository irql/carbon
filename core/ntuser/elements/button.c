


#define NTUSER_INTERNAL
#include <carbsup.h>
#include "../usersup.h"
#include "../ntuser.h"

BOOLEAN
NtClassButtonBaseProc(
    _In_ PKWND   Window,
    _In_ ULONG32 MessageId,
    _In_ ULONG64 Param1,
    _In_ ULONG64 Param2
)
{
    Param1;
    Param2;

    PDC Context;
    ULONG32 Width;
    ULONG32 Height;
    BOOLEAN PaintAlreadyBegan;
    RECT Clip;

    Width = Window->FrontContext->ClientArea.Right - Window->FrontContext->ClientArea.Left;
    Height = Window->FrontContext->ClientArea.Bottom - Window->FrontContext->ClientArea.Top;

    switch ( MessageId ) {
    case WM_PAINT:;
        PaintAlreadyBegan = Window->PaintBegan;

        NtDdiBeginPaint( &Context,
                         Window );

        for ( ULONG32 i = 0; i < Width * Height; i++ ) {
            ( ( ULONG32* )Context->DeviceSpecific )[ i ] = 0xFFC3C3C3;
        }

        for ( ULONG32 i = 0; i < Width - 1; i++ ) {
            ( ( ULONG32* )Context->DeviceSpecific )[ i ] = 0xFFFFFFFF;
        }

        for ( ULONG32 i = 0; i < Width; i++ ) {
            ( ( ULONG32* )Context->DeviceSpecific )[ ( Height - 1 ) * Width + i ] = 0xFF000000;
        }

        for ( ULONG32 i = 0; i < Width - 2; i++ ) {
            ( ( ULONG32* )Context->DeviceSpecific )[ ( Height - 2 ) * Width + i + 1 ] = 0xFF828282;
        }

        for ( ULONG32 i = 0; i < Height - 1; i++ ) {
            ( ( ULONG32* )Context->DeviceSpecific )[ Width * i ] = 0xFFFFFFFF;
        }

        for ( ULONG32 i = 0; i < Height; i++ ) {
            ( ( ULONG32* )Context->DeviceSpecific )[ Width * ( i + 1 ) - 1 ] = 0xFF000000;
        }

        for ( ULONG32 i = 0; i < Height - 2; i++ ) {
            ( ( ULONG32* )Context->DeviceSpecific )[ Width * ( i + 2 ) - 2 ] = 0xFF828282;
        }

        //Clip.Left = 2;
        //Clip.Top = 2;
        //Clip.Right = Width - 4;
        //Clip.Bottom = Height - 4;

        NtSystemFont->Engine->Extent( NtSystemFont,
                                      Context,
                                      Window->WindowInfo.Name,
                                      &Clip );

        RECT Clip2;

        Clip2.Left = ( Width - Clip.Right ) / 2; // min 2
        Clip2.Top = ( Height - Clip.Bottom ) / 2;
        Clip2.Right = Width - Clip2.Left;
        Clip2.Bottom = Height - Clip2.Top;

        NtSystemFont->Engine->Render( NtSystemFont,
                                      Context,
                                      Window->WindowInfo.Name,
                                      &Clip2,
                                      0xFF000000 );

        if ( !PaintAlreadyBegan ) {

            NtDdiEndPaint( Window );
        }

        break;
    default:
        break;
    }

    return TRUE;
}
