/*++

Module ObjectName:

	list.c

Abstract:

	Linked list support procedures.

--*/

#include <carbsup.h>

VOID
KeInsertListEntry(
	__in LIST_ENTRY* ListHead,
	__in LIST_ENTRY* EntryToInsert
	)

{
	LIST_ENTRY* LastEntry = ListHead;

	while (LastEntry->Flink != ListHead)
		LastEntry = LastEntry->Flink;

	EntryToInsert->Flink = LastEntry->Flink;
	EntryToInsert->Blink = ListHead->Blink;

	LastEntry->Flink = EntryToInsert;
	ListHead->Blink = EntryToInsert;
}

VOID
KeRemoveListEntry(
	__in LIST_ENTRY* EntryToRemove
	)

{

	EntryToRemove->Blink->Flink = EntryToRemove->Flink;
	EntryToRemove->Flink->Blink = EntryToRemove->Blink;

	EntryToRemove->Flink = EntryToRemove;
	EntryToRemove->Blink = EntryToRemove;
}

VOID
KeInitializeListHead(
	__in LIST_ENTRY* EntryToInit
	)

{

	EntryToInit->Flink = EntryToInit;
	EntryToInit->Blink = EntryToInit;
}
