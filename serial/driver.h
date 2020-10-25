

#include <carbsup.h>

typedef struct _COM_PORT {
	USHORT Port;
	BOOLEAN InUse;
} COM_PORT, *PCOM_PORT;

EXTERN COM_PORT COM[ ];

#define COM_DATA_REG 0
#define COM_INTERRUPT_ENABLE_REG 1

#define COM_LSB_BAUD_RATE 0
#define COM_MSB_BAUD_RATE 1

#define COM_INT_IDENT_FIFO_CR 2
#define COM_LINE_CONTROL_REG 3 //MSB IS THE DLAB
#define COM_MODEM_CONTROL_REG 4
#define COM_LINE_STATUS_REG 5
#define COM_MODEM_STATUS_REG 6
#define COM_SCRATCH_REG 7

#define COM_LC_DLAB		( 1 << 7 )

#define COM_LS_DR		( 1 << 0 )
#define COM_LS_OE		( 1 << 0 )
#define COM_LS_PE		( 1 << 2 )
#define COM_LS_FE		( 1 << 3 )
#define COM_LS_BI		( 1 << 4 )
#define COM_LS_THRE		( 1 << 5 )
#define COM_LS_TEMT		( 1 << 6 )
#define COM_LS_ER_INP	( 1 << 7 )

#define COM_LC_DB_5 0
#define COM_LC_DB_6 1
#define COM_LC_DB_7 2
#define COM_LC_DB_8 3

#define COM_LC_SB_1 0
#define COM_LC_SB_2 (1 << 2)

// fuck parity macros & ie macros

VOID
SerialInit(
	__in ULONG32 SerialPort
);

VOID
SerialWrite(
	__in ULONG32 Port,
	__in ULONG32 Length,
	__in PVOID Buffer
);

VOID
SerialRead(
	__in ULONG32 Port,
	__in ULONG32 Length,
	__in PVOID Buffer
);