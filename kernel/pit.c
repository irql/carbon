/*++

Module ObjectName:

	pit.c

Abstract:

	PIT procedures for bootstrapping.

--*/

#include <carbsup.h>
#include "hal.h"

VOID
HalPitDelayInternal(
	__in USHORT Milliseconds
);

VOID
HalPitDelay(
	__in USHORT Milliseconds
)
{

	while ( Milliseconds > 32 ) {
		Milliseconds -= 32;
		HalPitDelayInternal( 32 );
	}

	if ( Milliseconds != 0 ) {

		HalPitDelayInternal( Milliseconds );
	}

}

VOID
HalPitDelayInternal(
	__in USHORT Milliseconds
)
{

	//32ms max. (ish)

	USHORT Divisor = ( USHORT )( ( 1193181 / 1000 ) * Milliseconds );

	__outbyte( PIT_IOPORT_MODE_REGISTER, 0x30 );
	__outbyte( PIT_IOPORT_CHANNEL0_DATA, ( UCHAR )( Divisor ) );
	__outbyte( PIT_IOPORT_CHANNEL0_DATA, ( UCHAR )( Divisor >> 8 ) );

	while ( 1 ) {

		__outbyte( PIT_IOPORT_MODE_REGISTER, 0xe2 );

		if ( __inbyte( PIT_IOPORT_CHANNEL0_DATA ) & ( 1 << 7 ) )
			break;
	}

	return;
}

