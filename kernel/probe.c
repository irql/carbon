/*++

Module ObjectName:

	probe.c

Abstract:

	Memory probing.

--*/


#include <carbsup.h>

NTSTATUS
KeProbeForRead(
	__in PVOID Address,
	__in ULONG Length
	)
{
	Address;
	Length;

	return STATUS_SUCCESS;
}

NTSTATUS
KeProbeForWrite(
	__in PVOID Address,
	__in ULONG Length
	)
{
	Address;
	Length;

	return STATUS_SUCCESS;
}

