
#include "driver.h"

BOOLEAN
SerialReadyToSend(
	__in ULONG32 Port
)
{

	return ( __inbyte( COM[ Port ].Port + COM_LINE_STATUS_REG ) & COM_LS_THRE ) == COM_LS_THRE;
}

VOID
SerialWriteByte(
	__in ULONG32 Port,
	__in CHAR Byte
)
{

	while ( !SerialReadyToSend( Port ) )
		;

	__outbyte( COM[ Port ].Port + COM_DATA_REG, Byte );
}

VOID
SerialWrite(
	__in ULONG32 Port,
	__in ULONG32 Length,
	__in PVOID Buffer
)
{

	for ( ULONG32 i = 0; i < Length; i++ ) {

		SerialWriteByte( Port, ( ( UCHAR* )Buffer )[ i ] );
	}
}