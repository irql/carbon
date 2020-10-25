


#pragma once


NTSYSAPI
PVOID
ExAllocatePoolWithTag(
	__in ULONG NumberOfBytes,
	__in ULONG PoolTag
);

NTSYSAPI
VOID
ExFreePoolWithTag(
	__in PVOID Pool,
	__in ULONG PoolTag
);

