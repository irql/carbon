/*++

Module ObjectName:

	probe.c

Abstract:

	Memory probing.

--*/


#include <carbsup.h>

VOID
KeProbeForRead(
	__in PVOID Address,
	__in ULONG Length
)
{
#if 0
	if ( !MmIsAddressRangeValid( Address, Length ) ) {

		KeRaiseException( STATUS_ACCESS_VIOLATION );
	}
#endif
	Address;
	Length;

	return;
}

VOID
KeProbeForWrite(
	__in PVOID Address,
	__in ULONG Length
)
{
	Address;
	Length;

	return;
}

