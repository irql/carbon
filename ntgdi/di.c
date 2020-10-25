

#include "driver.h"
#include "wnd.h"

VOID
NtGdiDiInsert(
	__in PKWINDOW Window,
	__in PDI_BASE DiToInsert
)
{

	PDI_BASE DiFlink = Window->DiFlink;

	if ( DiFlink == NULL ) {

		Window->DiFlink = DiToInsert;
		return;
	}

	while ( DiFlink->DiFlink != NULL ) {

		DiFlink = DiFlink->DiFlink;
	}

	DiFlink->DiFlink = DiToInsert;
}