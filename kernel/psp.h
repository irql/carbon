


#pragma once


VOID
PspInsertProcess(
	__in PKPROCESS Process
);

VOID
PspRemoveProcess(
	__in PKPROCESS Process
);

PVAD
PspAllocateVad(

);

VOID
PspFreeVad(
	__in PVAD VadToFree
);

VOID
PspInsertVad(
	__in PKPROCESS Process,
	__in PVAD      VadToInsert
);

VOID
PspRemoveVad(
	__in PKPROCESS Process,
	__in PVAD      VadToRemove
);

PVAD
PspFindVad(
	__in PKPROCESS Process,
	__in PWSTR     RangeName
);
