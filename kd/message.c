


#include "com.h"

VOID
KdCmdMessage(
	__in PKD_CMDR_MESSAGE Message
)
{

	KdPrint( L"%s\n", Message->Message );
}
