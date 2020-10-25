


#include "driver.h"
#include "wnd.h"

KSPIN_LOCK g_WindowListLock;

VOID
NtGdiWindowInsert(
	__in PKWINDOW Insert
)
{
	PKWINDOW First = g_RootWindow;

	while ( First->Next != NULL ) {

		First = First->Next;
	}

	First->Next = Insert;

	//PKWINDOW First = g_RootWindow->Next;

	//g_RootWindow->Next = Insert;
	//Insert->Next = First;
}

VOID
NtGdiWindowRemove(
	__in PKWINDOW Remove
)
{
	PKWINDOW First = g_RootWindow;

	while ( First->Next != Remove ) {

		First = First->Next;
	}

	First->Next = Remove->Next;
	Remove->Next = NULL;
}

