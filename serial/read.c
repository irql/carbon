
#include "driver.h"

BOOLEAN
SerialReadyToRecieve(
	__in ULONG32 Port
)
{

	return ( __inbyte( COM[ Port ].Port + COM_LINE_STATUS_REG ) & COM_LS_DR ) == COM_LS_DR;
}

UCHAR
SerialReadByte(
	__in ULONG32 Port
)
{
	while ( !SerialReadyToRecieve( Port ) )
		;

	return __inbyte( COM[ Port ].Port + COM_DATA_REG );
}

VOID
SerialRead(
	__in ULONG32 Port,
	__in ULONG32 Length,
	__in PVOID Buffer
)
{

	for ( ULONG32 i = 0; i < Length; i++ ) {

		( ( UCHAR* )Buffer )[ i ] = SerialReadByte( Port );
	}
}


