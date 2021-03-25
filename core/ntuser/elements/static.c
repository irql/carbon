


#define NTUSER_INTERNAL
#include <carbsup.h>
#include "../usersup.h"
#include "../ntuser.h"

BOOLEAN
NtClassStaticBaseProc(
    _In_ PKWND   Window,
    _In_ ULONG32 MessageId,
    _In_ ULONG64 Param1,
    _In_ ULONG64 Param2
)
{
    Param1;
    Param2;

    PDC Context;
    BOOLEAN PaintAlreadyBegan;
    ULONG32 Width;
    ULONG32 Height;
    RECT Clip;

    Width = Window->FrontContext->ClientArea.Right - Window->FrontContext->ClientArea.Left;
    Height = Window->FrontContext->ClientArea.Bottom - Window->FrontContext->ClientArea.Top;

    switch ( MessageId ) {
    case WM_PAINT:;
        PaintAlreadyBegan = Window->PaintBegan;

        NtDdiBeginPaint( &Context,
                         Window );

        Clip.Left = 0;
        Clip.Right = Width;
        Clip.Top = 0;
        Clip.Bottom = Height;
        NtSystemFont->Engine->Render( NtSystemFont,
                                      Context,
                                      Window->WindowInfo.Name,
                                      &Clip,
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
