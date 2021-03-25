


#include <carbsup.h>
#include "i8042.h"

UCHAR I8042KeyboardFlags = 0;

UCHAR ScanTable[ 128 ] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8',  /* 9 */
    '9', '0', '-', '=', '\b',   /* Backspace */
    '\t',           /* Tab */
    'q', 'w', 'e', 'r', /* 19 */
    't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',   /* Enter key */
    0,          /* 29   - Control */
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',   /* 39 */
    '\'', '`', 0,       /* Left shift */
    '\\', 'z', 'x', 'c', 'v', 'b', 'n',         /* 49 */
    'm', ',', '.', '/', 0,              /* Right shift */
    '*',
    0,  /* Alt */
    ' ',    /* Space bar */
    0,  /* Caps lock */
    0,  /* 59 - F1 key ... > */
    0, 0, 0, 0, 0, 0, 0, 0,
    0,  /* < ... F10 */
    0,  /* 69 - Num lock*/
    0,  /* Scroll Lock */
    0,  /* Home key */
    0,  /* Up Arrow */
    0,  /* Page Up */
    '-',
    0,  /* Left Arrow */
    0,
    0,  /* Right Arrow */
    '+',
    0,  /* 79 - End key*/
    0,  /* Down Arrow */
    0,  /* Page Down */
    0,  /* Insert Key */
    0,  /* Delete Key */
    0, 0, 0,
    0,  /* F11 Key */
    0,  /* F12 Key */
    0,  /* All other keys are undefined */
};

UCHAR ScanTableShift[ 128 ] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*',  /* 9 */
    '(', ')', '_', '+', '\b',   /* Backspace */
    '\t',           /* Tab */
    'Q', 'W', 'E', 'R', /* 19 */
    'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',   /* Enter key */
    0,          /* 29   - Control */
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',   /* 39 */
    '\"', '~', 0,       /* Left shift */
    '|', 'Z', 'X', 'C', 'V', 'B', 'N',          /* 49 */
    'M', '<', '>', '?', 0,              /* Right shift */
    '*',
    0,  /* Alt */
    ' ',    /* Space bar */
    0,  /* Caps lock */
    0,  /* 59 - F1 key ... > */
    0, 0, 0, 0, 0, 0, 0, 0,
    0,  /* < ... F10 */
    0,  /* 69 - Num lock*/
    0,  /* Scroll Lock */
    0,  /* Home key */
    0,  /* Up Arrow */
    0,  /* Page Up */
    '-',
    0,  /* Left Arrow */
    0,
    0,  /* Right Arrow */
    '+',
    0,  /* 79 - End key*/
    0,  /* Down Arrow */
    0,  /* Page Down */
    0,  /* Insert Key */
    0,  /* Delete Key */
    0, 0, 0,
    0,  /* F11 Key */
    0,  /* F12 Key */
    0,  /* All other keys are undefined */
};

ULONG64
I8042ScanToVirtual(
    _In_ PUCHAR  Scancodes,
    _In_ ULONG32 Codes
)
{
    Codes;

    if ( I8042KeyboardFlags & KEY_FLAG_SHIFT ) {

        return ScanTableShift[ Scancodes[ 0 ] ];
    }
    else {

        return ScanTable[ Scancodes[ 0 ] ];
    }
}

BOOLEAN
I8042KeyboardInterrupt(
    _In_ PKINTERRUPT Interrupt
)
{
    Interrupt;

    UCHAR     Scancode[ 4 ];
    KEY_STATE KeyState;

    Scancode[ 0 ] = __inbyte( I8042_CONTROLLER_CMD1 );
    KeyState = ( Scancode[ 0 ] & 0x80 ) == 0x80 ? KeyStateRelease : KeyStatePress;
    Scancode[ 0 ] &= ~0x80;

    if ( Scancode[ 0 ] == 0x1f ) {
        __debugbreak( );
    }

    switch ( Scancode[ 0 ] ) {
    case 0x2A:
        I8042KeyboardFlags |= KEY_FLAG_SHIFT;
        break;
    case 0xAA:
        I8042KeyboardFlags &= ~KEY_FLAG_SHIFT;
        break;
    default:
        NtSendSystemMessage( KeyState == KeyStatePress ? WM_KEYDOWN : WM_KEYUP,
                             I8042ScanToVirtual( Scancode, 1 ),
                             I8042KeyboardFlags );
        break;
    }

    return TRUE;
}
