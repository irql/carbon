


#include <carbsup.h>
#include "ki.h"
#include "psp.h"

VOID
PspInsertProcess(
	__in PKPROCESS Process
)
{

	KeInsertListEntry( &KiSystemProcess->ActiveProcessLinks, &Process->ActiveProcessLinks );
}

VOID
PspRemoveProcess(
	__in PKPROCESS Process
)
{

	KeRemoveListEntry( &Process->ActiveProcessLinks );
}
