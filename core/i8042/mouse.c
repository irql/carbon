


#include <carbsup.h>
#include "i8042.h"

DLLIMPORT PULONG32 MappedFramebuffer;

UCHAR   I8042MouseCycle = 0;
UCHAR   I8042MouseByte[ 4 ];
UCHAR   I8042PreviousByte0 = 0;

LONG32  I8042MousePositionX = 0;
LONG32  I8042MousePositionY = 0;

ULONG64 I8042MouseType;

BOOLEAN
I8042MouseInterrupt(
    _In_ PKINTERRUPT Interrupt
)
{
    Interrupt;

    I8042MouseByte[ I8042MouseCycle ] = __inbyte( I8042_REG_OUTPUT );

    ULONG32 Width;
    ULONG32 Height;
    ULONG32 BitDepth;

    switch ( I8042MouseType ) {

    case I8042MouseStandard:
        switch ( I8042MouseCycle ) {
        case 0:

            if ( I8042MouseByte[ 0 ] & I8042_ST_COMMAND ) {

                I8042MouseCycle++;
            }
            break;
        case 1:

            I8042MouseCycle++;
            break;
        case 2:

            if ( I8042MouseByte[ 0 ] & ( 0x80 | 0x40 ) ) {

                I8042MouseCycle = 0;
                break;
            }

            if ( I8042MouseByte[ 0 ] & 0x20 ) {

                I8042MousePositionY -= ( ( ULONG32 )I8042MouseByte[ 2 ] | 0xFFFFFF00 );
            }
            else {

                I8042MousePositionY -= ( ( ULONG32 )I8042MouseByte[ 2 ] );
            }

            if ( I8042MouseByte[ 0 ] & 0x10 ) {

                I8042MousePositionX += ( ( ULONG32 )I8042MouseByte[ 1 ] | 0xFFFFFF00 );
            }
            else {

                I8042MousePositionX += ( ( ULONG32 )I8042MouseByte[ 1 ] );
            }

            if ( I8042MousePositionX < 0 ) {

                I8042MousePositionX = 0;
            }

            if ( I8042MousePositionY < 0 ) {

                I8042MousePositionY = 0;
            }

            NtGetMode( &Width, &Height, &BitDepth );

            if ( I8042MousePositionX > ( LONG32 )Width ) {

                I8042MousePositionX = Width;
            }

            if ( I8042MousePositionY > ( LONG32 )Height ) {

                I8042MousePositionY = Height;
            }

            if ( !( I8042PreviousByte0 & 1 ) ) {

                //g_CursorVisible = TRUE;
                //g_TypingCursor = FALSE;
            }

            NtSetCursorPosition( I8042MousePositionX,
                                 I8042MousePositionY );

            if ( I8042MouseByte[ 0 ] & 1 ) {

                NtSendSystemMessage( WM_LMOUSEDOWN, ( ULONG64 )I8042MousePositionX, ( ULONG64 )I8042MousePositionY );
            }
            else if ( I8042PreviousByte0 & 1 ) {

                NtSendSystemMessage( WM_LMOUSEUP, ( ULONG64 )I8042MousePositionX, ( ULONG64 )I8042MousePositionY );
            }

            if ( I8042MouseByte[ 0 ] & 2 ) {

                NtSendSystemMessage( WM_RMOUSEDOWN, ( ULONG64 )I8042MousePositionX, ( ULONG64 )I8042MousePositionY );
            }
            else if ( I8042PreviousByte0 & 2 ) {

                NtSendSystemMessage( WM_RMOUSEUP, ( ULONG64 )I8042MousePositionX, ( ULONG64 )I8042MousePositionY );
            }

            if ( I8042MouseByte[ 0 ] & 4 ) {

            }
            else if ( I8042PreviousByte0 & 4 ) {

            }

            I8042PreviousByte0 = I8042MouseByte[ 0 ];

            I8042MouseCycle = 0;
            break;
        default:
            break;
        }
        break;
    case I8042MouseScroll:
        switch ( I8042MouseCycle ) {
        case 0:

            if ( I8042MouseByte[ 0 ] & I8042_ST_COMMAND ) {

                I8042MouseCycle++;
            }
            break;
        case 1:

            I8042MouseCycle++;
            break;
        case 2:

            I8042MouseCycle++;
            break;
        case 3:

            if ( I8042MouseByte[ 0 ] & ( 0x80 | 0x40 ) ) {

                I8042MouseCycle = 0;
                break;
            }

            if ( I8042MouseByte[ 0 ] & 0x20 ) {

                I8042MousePositionY -= ( ( ULONG32 )I8042MouseByte[ 2 ] | 0xFFFFFF00 );
            }
            else {

                I8042MousePositionY -= ( ( ULONG32 )I8042MouseByte[ 2 ] );
            }

            if ( I8042MouseByte[ 0 ] & 0x10 ) {

                I8042MousePositionX += ( ( ULONG32 )I8042MouseByte[ 1 ] | 0xFFFFFF00 );
            }
            else {

                I8042MousePositionX += ( ( ULONG32 )I8042MouseByte[ 1 ] );
            }

            if ( I8042MouseByte[ 3 ] != 0 ) {

                NtSendSystemMessage( WM_VSCROLL,
                    ( ULONG64 )( ( LONG64 )( CHAR )I8042MouseByte[ 3 ] ),
                                     0 );
            }

            if ( I8042MousePositionX < 0 ) {

                I8042MousePositionX = 0;
            }

            if ( I8042MousePositionY < 0 ) {

                I8042MousePositionY = 0;
            }

            NtGetMode( &Width, &Height, &BitDepth );

            if ( I8042MousePositionX > ( LONG32 )Width ) {

                I8042MousePositionX = Width;
            }

            if ( I8042MousePositionY > ( LONG32 )Height ) {

                I8042MousePositionY = Height;
            }

            if ( !( I8042PreviousByte0 & 1 ) ) {

                //g_CursorVisible = TRUE;
                //g_TypingCursor = FALSE;
            }

            NtSetCursorPosition( I8042MousePositionX,
                                 I8042MousePositionY );

            if ( I8042MouseByte[ 0 ] & 1 ) {

                NtSendSystemMessage( WM_LMOUSEDOWN, ( ULONG64 )I8042MousePositionX, ( ULONG64 )I8042MousePositionY );
            }
            else if ( I8042PreviousByte0 & 1 ) {

                NtSendSystemMessage( WM_LMOUSEUP, ( ULONG64 )I8042MousePositionX, ( ULONG64 )I8042MousePositionY );
            }

            if ( I8042MouseByte[ 0 ] & 2 ) {

                NtSendSystemMessage( WM_RMOUSEDOWN, ( ULONG64 )I8042MousePositionX, ( ULONG64 )I8042MousePositionY );
            }
            else if ( I8042PreviousByte0 & 2 ) {

                NtSendSystemMessage( WM_RMOUSEUP, ( ULONG64 )I8042MousePositionX, ( ULONG64 )I8042MousePositionY );
            }

            if ( I8042MouseByte[ 0 ] & 4 ) {


            }
            else if ( I8042PreviousByte0 & 4 ) {

            }

            I8042PreviousByte0 = I8042MouseByte[ 0 ];

            I8042MouseCycle = 0;
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    return TRUE;
}

VOID
I8042MouseSetSampleRate(
    _In_ UCHAR SampleRate
)
{

    I8042MouseWrite( I8042_CMD_SET_SAMPLE_RATE );
    I8042MouseRead( );

    I8042MouseWrite( SampleRate );
    I8042MouseRead( );

}
