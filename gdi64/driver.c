


#include <carbsup.h>

#define NTSYSCALLAPI DECLSPEC(DLLEXPORT)
#include "../ntgdi/ntgdi.h"

VOID
DllMain(
	__in ULONG64 Base,
	__in ULONG32 Reason
)
{
	Base;

	switch ( Reason ) {

	case REASON_DLL_LOAD:

		break;
	case REASON_DLL_UNLOAD:

		break;
	}

	return;
}


