


#include <carbsup.h>
#include "../hal/halp.h"
#include "ki.h"

PKPCB*  KiLogicalProcessor = NULL;
ULONG64 KiLogicalProcessorCount = 0;

PKPCB
KiCreatePcb(

)
{
    PKPCB Processor;

    if ( KiLogicalProcessor == NULL ) {

        KiLogicalProcessor = ( PKPCB* )MmAllocatePoolWithTag( NonPagedPool, sizeof( PKPCB ) * HalLocalApicCount, KE_TAG );
    }

    Processor = MmAllocatePoolWithTag( NonPagedPoolZeroed, sizeof( KPCB ), KE_TAG );
    Processor->ProcessorNumber = KiLogicalProcessorCount;

    KiLogicalProcessor[ KiLogicalProcessorCount++ ] = Processor;

    return Processor;
}

PKPCB
KeQueryProcessorByNumber(
    _In_ ULONG64 ProcessorNumber
)
{
    if ( ProcessorNumber > KiLogicalProcessorCount ) {
        return NULL;
    }
    return KiLogicalProcessor[ ProcessorNumber ];
}

ULONG64
KeQueryProcessorCount(

)
{
    return KiLogicalProcessorCount;
}
