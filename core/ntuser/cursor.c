


#define NTUSER_INTERNAL
#include <carbsup.h>
#include "usersup.h"
#include "ntuser.h"

ULONG32 NtCursorPositionX = 0;
ULONG32 NtCursorPositionY = 0;

DLLIMPORT PULONG32 MappedFramebuffer;

VOID
NtSetCursorPosition(
    _In_ LONG32 posx,
    _In_ LONG32 posy
)
{
    //
    // it's signed because mouse drivers will use signed
    //

    if ( posx < 0 ) {
        posx = 0;
    }
    if ( posy < 0 ) {
        posy = 0;
    }

    if ( posx > NtScreenDC->ClientArea.Right ) {
        posx = NtScreenDC->ClientArea.Right;
    }
    if ( posy > NtScreenDC->ClientArea.Bottom ) {
        posy = NtScreenDC->ClientArea.Bottom;
    }

    if ( ( ULONG32 )posx != NtCursorPositionX ||
        ( ULONG32 )posy != NtCursorPositionY ) {

        NtCursorPositionX = posx;
        NtCursorPositionY = posy;

        NtDrawCursor( );
    }
}

VOID
NtGetCursorPosition(
    _Out_ ULONG32* posx,
    _Out_ ULONG32* posy
)
{
    __try {

        *posx = NtCursorPositionX;
        *posy = NtCursorPositionY;
    }
    __except ( EXCEPTION_EXECUTE_HANDLER ) {

        RtlRaiseException( STATUS_ACCESS_VIOLATION );
    }
}

VOID
NtDrawCursor(

)
{
    ULONG32 Cursor[ ] = {
        0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000,
        0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000,
        0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000,
        0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000,
    };

    NtDdiBlt( Composed,
              0,
              0,
              Composed->ClientArea.Right,
              Composed->ClientArea.Bottom,
              NtScreenDC,
              0,
              0 );

    NtDdiBltBits( Cursor,
                  0,
                  0,
                  4,
                  4,
                  NtScreenDC,
                  NtCursorPositionX,
                  NtCursorPositionY );
}
