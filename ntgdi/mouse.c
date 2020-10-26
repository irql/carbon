



#include "driver.h"
#include "i8042.h"

#include "svga.h"

UCHAR NtMouseCycle = 0;
UCHAR NtMouseByte[ 3 ];
UCHAR NtPreviousByte0 = 0;
POINT g_NtCursorPosition = { 0, 0 };
BOOLEAN g_CursorVisible = FALSE;

VOID
NtSendMousePacket(
	__in UCHAR Button,
	__in KEY_STATE State,
	__in PPOINT Position
)
{
	MOUSE_PACKET MousePacket;
	MousePacket.Button = Button;
	MousePacket.State = State;
	MousePacket.Position.x = Position->x;
	MousePacket.Position.y = Position->y;

	NtGdiMouseHandleClick( &MousePacket );
}

VOID
NtMouseUpdateIrq(
	__in PKTRAP_FRAME TrapFrame,
	__in PKPCR Processor
)
{

	TrapFrame;
	Processor;
	//KeEnterCriticalRegion();

	NtMouseByte[ NtMouseCycle ] = __inbyte( I8042_CONTROLLER_CMD1 );

	SIGNED_POINT Mouse = *( PSIGNED_POINT )&g_NtCursorPosition;

	switch ( NtMouseCycle ) {
	case 0:

		if ( NtMouseByte[ 0 ] & I8042_ST_V_BIT ) {

			NtMouseCycle++;
		}
		break;
	case 1:

		NtMouseCycle++;
		break;
	case 2:

		if ( NtMouseByte[ 0 ] & ( 0x80 | 0x40 ) ) {

			NtMouseCycle = 0;
			break;
		}

		if ( NtMouseByte[ 0 ] & 0x20 ) {

			Mouse.y -= ( ( ULONG32 )NtMouseByte[ 2 ] | 0xFFFFFF00 );
		}
		else {

			Mouse.y -= ( ( ULONG32 )NtMouseByte[ 2 ] );
		}

		if ( NtMouseByte[ 0 ] & 0x10 ) {

			Mouse.x += ( ( ULONG32 )NtMouseByte[ 1 ] | 0xFFFFFF00 );
		}
		else {

			Mouse.x += ( ( ULONG32 )NtMouseByte[ 1 ] );
		}

		if ( Mouse.x < 0 ) {

			Mouse.x = 0;
		}

		if ( Mouse.y < 0 ) {

			Mouse.y = 0;
		}

		if ( Mouse.x > ( signed )g_Basic.Width ) {

			Mouse.x = g_Basic.Width;
		}

		if ( Mouse.y > ( signed )g_Basic.Height ) {

			Mouse.y = g_Basic.Height;
		}

		//NtGdiMouseHandleMove( g_NtCursorPosition, *( PPOINT )&Mouse );

		g_NtCursorPosition.x = Mouse.x;
		g_NtCursorPosition.y = Mouse.y;

		if ( !( NtPreviousByte0 & 1 ) ) {

			g_CursorVisible = TRUE;
			g_TypingCursor = FALSE;
		}

		SvMoveCursor( g_CursorVisible, g_NtCursorPosition.x, g_NtCursorPosition.y );

		if ( NtMouseByte[ 0 ] & 1 ) {

			NtSendMousePacket( 1, KeyStatePress, &g_NtCursorPosition );
		}
		else if ( NtPreviousByte0 & 1 ) {

			NtSendMousePacket( 1, KeyStateRelease, &g_NtCursorPosition );
		}

		if ( NtMouseByte[ 0 ] & 2 ) {

			NtSendMousePacket( 2, KeyStatePress, &g_NtCursorPosition );
		}
		else if ( NtPreviousByte0 & 2 ) {

			NtSendMousePacket( 2, KeyStateRelease, &g_NtCursorPosition );
		}

		if ( NtMouseByte[ 0 ] & 4 ) {

			NtSendMousePacket( 3, KeyStatePress, &g_NtCursorPosition );
		}
		else if ( NtPreviousByte0 & 4 ) {

			NtSendMousePacket( 3, KeyStateRelease, &g_NtCursorPosition );
		}

		NtPreviousByte0 = NtMouseByte[ 0 ];
		/*
			we're only supporting I8042DeviceTypeMousePs2Standard which only sends 3 bytes.
		*/
		NtMouseCycle = 0;

		break;
	case 3:
	case 4:
	default:
		break;
	}

	//KeLeaveCriticalRegion();
}

VOID
FORCEINLINE
NtMouseWait(
	__in BOOLEAN Write
)
{
	ULONG32 TimeOut = 100000;

	if ( Write ) {
		while ( TimeOut-- ) {
			if ( ( __inbyte( I8042_CONTROLLER_CMD2 ) & I8042_ST_CMD2_BIT ) == 1 )
				return;
		}

		//DbgPrint("ps2 write timeout.\n");
		return;
	}
	else {
		while ( TimeOut-- ) {
			if ( ( __inbyte( I8042_CONTROLLER_CMD2 ) & I8042_ST_CMD1_BIT ) == 0 )
				return;
		}

		//DbgPrint("ps2 read timeout.\n");
		return;
	}
}

VOID
FORCEINLINE
NtMouseWrite(
	__in UCHAR Value
)
{
	NtMouseWait( 1 );
	__outbyte( I8042_CONTROLLER_CMD2, I8042_CMD_WRITE_BYTE );
	NtMouseWait( 1 );
	__outbyte( I8042_CONTROLLER_CMD1, Value );
}

UCHAR
FORCEINLINE
NtMouseRead(

)
{
	NtMouseWait( 0 );
	return __inbyte( I8042_CONTROLLER_CMD1 );
}

VOID
NtMouseSetSampleRate(
	__in UCHAR SampleRate
)
{

	NtMouseWrite( I8042_CMD_SET_SAMPLE_RATE );
	NtMouseRead( );

	NtMouseWrite( SampleRate );
	NtMouseRead( );

}

VOID
NtMouseInstall(

)
{
	KeEnterCriticalRegion( );

	HalIdtInstallHandler( 0x60, NtMouseUpdateIrq );

	REDIRECTION_ENTRY RedirectionEntry = { 0 };

	RedirectionEntry.InterruptVector = 0x60;
	RedirectionEntry.DeliveryMode = DeliveryModeEdge;
	RedirectionEntry.Mask = 0;
	RedirectionEntry.DestinationMode = DestinationModePhysical;
	RedirectionEntry.PinPolarity = 0;
	RedirectionEntry.RemoteIrr = 0;
	RedirectionEntry.Mask = 0;

	PKPCR Processor0;
	KeQueryLogicalProcessor( 0, &Processor0 );

	RedirectionEntry.Destination = Processor0->AcpiId;
	HalIoApicRedirectIrq( 12, &RedirectionEntry );

	NtMouseWait( 1 );
	__outbyte( I8042_CONTROLLER_CMD2, 0xA8 );

	NtMouseWait( 1 );
	__outbyte( I8042_CONTROLLER_CMD2, 0x20 );

	NtMouseWait( 0 );
	UCHAR Status = ( __inbyte( I8042_CONTROLLER_CMD1 ) | ( 1 << 1 ) ) & ~( 1 << 5 );

	__inbyte( I8042_CONTROLLER_CMD1 );//Bochs sends 0xD8

	NtMouseWait( 1 );
	__outbyte( I8042_CONTROLLER_CMD2, 0x60 );

	NtMouseWait( 1 );
	__outbyte( I8042_CONTROLLER_CMD1, Status );
	NtMouseRead( );//might generate ACK

	NtMouseWrite( I8042_CMD_SET_DEFAULTS );
	NtMouseRead( );
	/*
	NtMouseWrite(I8042_CMD_GET_MOUSE_ID);
	NtMouseRead();

	if (NtMouseRead() != I8042DeviceTypeMousePs2Standard) {

		DbgPrint("unrecognised mouse id.\n");
	}*/

	NtMouseWrite( I8042_CMD_ENABLE_PACKET_STREAMING );
	NtMouseRead( );

	KeLeaveCriticalRegion( );
}

