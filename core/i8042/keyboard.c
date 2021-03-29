


#include <carbsup.h>
#include "i8042.h"
#include "../ntuser/usersup.h"

UCHAR I8042KeyboardFlags = 0;

UCHAR ScanTable[ 128 ] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8',
    '9', '0', '-', '=', VK_BACK,
    VK_TAB,
    'q', 'w', 'e', 'r',
    't', 'y', 'u', 'i', 'o', 'p', '[', ']', VK_ENTER,
    VK_CTRL,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
    '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n',
    'm', ',', '.', '/', 0,              /* Right shift */
    '*',
    VK_ALT,
    ' ',
    0,  /* Caps lock */
    VK_F1,
    VK_F2, VK_F3, VK_F4, VK_F5, VK_F6, VK_F7, VK_F8, VK_F9,
    VK_F10,
    0,  /* 69 - Num lock*/
    0,  /* Scroll Lock */
    VK_HOME,
    VK_UP,
    VK_PGUP,
    '-',
    VK_LEFT,
    0,
    VK_RIGHT,
    '+',
    VK_END,
    VK_DOWN,
    VK_PGDOWN,
    VK_INS,
    VK_DEL,
    0, 0, '\\', // on my keyboard this is \.
    VK_F11,
    VK_F12,
    0,  /* All other keys are undefined */
};

UCHAR ScanTableShift[ 128 ] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*',  /* 9 */
    '(', ')', '_', '+', VK_BACK,
    VK_TAB,
    'Q', 'W', 'E', 'R', /* 19 */
    'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', VK_ENTER,
    VK_CTRL,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',   /* 39 */
    '\"', '~', 0,       /* Left shift */
    '|', 'Z', 'X', 'C', 'V', 'B', 'N',          /* 49 */
    'M', '<', '>', '?', 0,              /* Right shift */
    '*',
    VK_ALT,
    ' ',
    0,  /* Caps lock */
    VK_F1,
    VK_F2, VK_F3, VK_F4, VK_F5, VK_F6, VK_F7, VK_F8, VK_F9,
    VK_F10,
    0,  /* 69 - Num lock*/
    0,  /* Scroll Lock */
    VK_HOME,
    VK_UP,
    VK_PGUP,
    '-',
    VK_LEFT,
    0,
    VK_RIGHT,
    '+',
    VK_END,
    VK_DOWN,
    VK_PGDOWN,
    VK_INS,
    VK_DEL,
    0, 0, '|',
    VK_F11,
    VK_F12,
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

    switch ( Scancode[ 0 ] ) {
    case 0x2A:
        I8042KeyboardFlags |= KEY_FLAG_SHIFT;
        break;
    case 0xAA:
        I8042KeyboardFlags &= ~KEY_FLAG_SHIFT;
        break;
    default:
        Scancode[ 0 ] &= ~0x80;

        NtSendSystemMessage( KeyState == KeyStatePress ? WM_KEYDOWN : WM_KEYUP,
                             I8042ScanToVirtual( Scancode, 1 ),
                             I8042KeyboardFlags );
        break;
    }

    return TRUE;
}
