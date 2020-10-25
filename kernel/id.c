/*++

Module ObjectName:

	id.c

Abstract:

	Supplies identifiers.

--*/

#include <carbsup.h>
#include "ke.h"

ULONG32
KiGetUniqueIdentifier(

)
{
	STATIC ULONG32 IdenfitierIncrementor = 4;

	ULONG32 UniqueId = IdenfitierIncrementor;
	IdenfitierIncrementor += 4;

	return UniqueId;
}
