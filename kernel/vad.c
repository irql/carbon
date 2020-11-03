


#include <carbsup.h>
#include "ki_struct.h"

PVAD
PspAllocateVad(

)
{
	PVAD Vad = ExAllocatePoolWithTag( sizeof( VAD ), TAGEX_VAD );
	Vad->Next = NULL;

	return Vad;
}

VOID
PspFreeVad(
	__in PVAD VadToFree
)
{

	ExFreePoolWithTag( VadToFree, TAGEX_VAD );
}

VOID
PspInsertVad(
	__in PKPROCESS Process,
	__in PVAD      VadToInsert
)
{

	PVAD LastVad = &Process->VadTree;

	while ( LastVad->Next != NULL ) {

		LastVad = LastVad->Next;
	}

	LastVad->Next = VadToInsert;
}

VOID
PspRemoveVad(
	__in PKPROCESS Process,
	__in PVAD      VadToRemove
)
{

	PVAD ParentVad = &Process->VadTree;

	while ( ParentVad->Next != VadToRemove ) {

		ParentVad = ParentVad->Next;
	}

	ParentVad->Next = VadToRemove->Next;
}

PVAD
PspFindVad(
	__in PKPROCESS Process,
	__in PWSTR     RangeName
)
{

	PVAD CurrentVad = &Process->VadTree;

	while ( CurrentVad != NULL ) {

		if ( lstrcmpW( RangeName, ( PCWSTR )CurrentVad->RangeName.Buffer ) == 0 ) {

			return CurrentVad;
		}

		CurrentVad = CurrentVad->Next;
	}

	return NULL;
}

