


#include "driver.h"
#include "i8042.h"

//https://gist.github.com/davazp/d2fde634503b2a5bc664

unsigned char kbdus[ 128 ] = {
	0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
	'9', '0', '-', '=', '\b',	/* Backspace */
	'\t',			/* Tab */
	'q', 'w', 'e', 'r',	/* 19 */
	't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
	0,			/* 29   - Control */
	'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
	'\'', '`', 0,		/* Left shift */
	'\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
	'm', ',', '.', '/',   0,				/* Right shift */
	'*',
	0,	/* Alt */
	' ',	/* Space bar */
	0,	/* Caps lock */
	0,	/* 59 - F1 key ... > */
	0,   0,   0,   0,   0,   0,   0,   0,
	0,	/* < ... F10 */
	0,	/* 69 - Num lock*/
	0,	/* Scroll Lock */
	0,	/* Home key */
	0,	/* Up Arrow */
	0,	/* Page Up */
	'-',
	0,	/* Left Arrow */
	0,
	0,	/* Right Arrow */
	'+',
	0,	/* 79 - End key*/
	0,	/* Down Arrow */
	0,	/* Page Down */
	0,	/* Insert Key */
	0,	/* Delete Key */
	0,   0,   0,
	0,	/* F11 Key */
	0,	/* F12 Key */
	0,	/* All other keys are undefined */
};

unsigned char kbdus_shift[ 128 ] = {
	0,  27, '!', '@', '#', '$', '%', '^', '&', '*',	/* 9 */
	'(', ')', '_', '+', '\b',	/* Backspace */
	'\t',			/* Tab */
	'Q', 'W', 'E', 'R',	/* 19 */
	'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',	/* Enter key */
	0,			/* 29   - Control */
	'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',	/* 39 */
	'\"', '~', 0,		/* Left shift */
	'|', 'Z', 'X', 'C', 'V', 'B', 'N',			/* 49 */
	'M', '<', '>', '?', 0,				/* Right shift */
	'*',
	0,	/* Alt */
	' ',	/* Space bar */
	0,	/* Caps lock */
	0,	/* 59 - F1 key ... > */
	0,   0,   0,   0,   0,   0,   0,   0,
	0,	/* < ... F10 */
	0,	/* 69 - Num lock*/
	0,	/* Scroll Lock */
	0,	/* Home key */
	0,	/* Up Arrow */
	0,	/* Page Up */
	'-',
	0,	/* Left Arrow */
	0,
	0,	/* Right Arrow */
	'+',
	0,	/* 79 - End key*/
	0,	/* Down Arrow */
	0,	/* Page Down */
	0,	/* Insert Key */
	0,	/* Delete Key */
	0, 0, 0,
	0,	/* F11 Key */
	0,	/* F12 Key */
	0,	/* All other keys are undefined */
};

UCHAR NtKeyboardFlags = 0;

VOID
NtSendKeyboardPacket(
	__in WCHAR Character,
	__in KEY_STATE State,
	__in UCHAR Flags
)
{
	if ( Character == 0 )
		return;

	KEYBOARD_PACKET KeyboardPacket;
	KeyboardPacket.Character = Character;
	KeyboardPacket.State = State;
	KeyboardPacket.Flags = Flags;

#if 0
	if ( FocusedObject->EventProcedure ) {

		FocusedObject->EventProcedure( FocusedObject, EVT_KEY_PRESS, &KeyboardPacket, NULL );
	}
#endif
}

WCHAR
NtTranslateScancodeToCharacter(
	__in UCHAR Scancode
)
{
	Scancode &= ~0x80;

	if ( NtKeyboardFlags & KEY_FLAG_SHIFT ) {

		return ( WCHAR )kbdus_shift[ Scancode ];
	}
	else {

		return ( WCHAR )kbdus[ Scancode ];
	}
}

VOID
NtKeyboardUpdateIrq(
	__in PKTRAP_FRAME TrapFrame,
	__in PKPCR Processor
)
{
	TrapFrame;
	Processor;

	UCHAR Scancode = __inbyte( I8042_CONTROLLER_CMD1 );

	switch ( Scancode ) {
	case 0x2A:
		NtKeyboardFlags |= KEY_FLAG_SHIFT;
		break;
	case 0xAA:
		NtKeyboardFlags &= ~KEY_FLAG_SHIFT;
		break;
	default:
		NtSendKeyboardPacket( NtTranslateScancodeToCharacter( Scancode ), ( Scancode & 0x80 ) ? KeyStateRelease : KeyStatePress, NtKeyboardFlags );

		break;
	}
}

VOID
NtKeyboardInstall(

)
{
	KeEnterCriticalRegion( );

	HalIdtInstallHandler( 0x61, NtKeyboardUpdateIrq );

	REDIRECTION_ENTRY RedirectionEntry = { 0 };

	RedirectionEntry.InterruptVector = 0x61;
	RedirectionEntry.DeliveryMode = DeliveryModeEdge;
	RedirectionEntry.Mask = 0;
	RedirectionEntry.DestinationMode = DestinationModePhysical;
	RedirectionEntry.PinPolarity = 0;
	RedirectionEntry.RemoteIrr = 0;
	RedirectionEntry.Mask = 0;

	PKPCR Processor0;
	KeQueryLogicalProcessor( 0, &Processor0 );

	RedirectionEntry.Destination = Processor0->AcpiId;
	HalIoApicRedirectIrq( 1, &RedirectionEntry );

	KeLeaveCriticalRegion( );
}
