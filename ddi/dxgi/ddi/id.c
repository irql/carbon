


#include <carbsup.h>
#include "../dxgi.h"
#include "ddi.h"

ULONG32
DdCreateUniqueId(

)
{
    STATIC ULONG32 LastId = 0;

    _InterlockedIncrement( ( volatile long* )&LastId );
    return LastId;
}
