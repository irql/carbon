


#include <carbsup.h>
#include "ke.h"
#include "psp.h"

EXTERN PKPROCESS KiSystemProcess;

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
