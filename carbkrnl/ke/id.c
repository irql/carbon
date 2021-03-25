


#include <carbsup.h>
#include "../hal/halp.h"
#include "ki.h"

ULONG64
KeGenerateUniqueId(

)
{
    STATIC KSPIN_LOCK IdLock = { 0 };
    STATIC ULONG64 UniqueId = 0;
    KIRQL PreviousIrql;

    KeAcquireSpinLock( &IdLock, &PreviousIrql );
    UniqueId += 4;
    // there is no interlockedadd :gorilla:
    //this is the easiest work-around
    KeReleaseSpinLock( &IdLock, PreviousIrql );

    return UniqueId;
}
