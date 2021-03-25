


#include <carbsup.h>
#include "../hal/halp.h"
#include "ki.h"

KIRQL
KeGetCurrentIrql(

)
{
    return __readcr8( );
}

VOID
KeRaiseIrql(
    _In_  KIRQL  NewIrql,
    _Out_ PKIRQL OldIrql
)
{
    *OldIrql = KeGetCurrentIrql( );

    if ( *OldIrql > NewIrql ) {

        return;
    }

    __writecr8( NewIrql );
}

VOID
KeLowerIrql(
    _In_ KIRQL NewIrql
)
{
    if ( NewIrql > KeGetCurrentIrql( ) ) {
        return;
    }

    __writecr8( NewIrql );
}
