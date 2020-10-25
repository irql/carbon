



#include "driver.h"


VOID
SerialInit(
	__in ULONG32 SerialPort
)
{

	__outbyte( COM[ SerialPort ].Port + COM_INTERRUPT_ENABLE_REG, 0 );

	__outbyte( COM[ SerialPort ].Port + COM_LINE_CONTROL_REG, COM_LC_DLAB );

	__outbyte( COM[ SerialPort ].Port + COM_LSB_BAUD_RATE, 1 );
	__outbyte( COM[ SerialPort ].Port + COM_MSB_BAUD_RATE, 0 );

	__outbyte( COM[ SerialPort ].Port + COM_LINE_CONTROL_REG, 3 );

	__outbyte( COM[ SerialPort ].Port + COM_INT_IDENT_FIFO_CR, 0xC7 );
	__outbyte( COM[ SerialPort ].Port + COM_MODEM_CONTROL_REG, 0xB );
}