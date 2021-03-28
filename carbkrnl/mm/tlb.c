


#include <carbsup.h>
#include "mi.h"
#include "../hal/halp.h"
#include "../ke/ki.h"

VOID
MmIpiAddressFlush(
    _In_ PVOID Address
)
{
    __invlpg( Address );
}

VOID
MmFlushAddress(
    _In_ PVOID Address
)
{
    struct {
        ULONG64 Pcid;
        PVOID   Address;
    } InvpcidDescriptor;

    if ( KeProcessorFeatureEnabled( KeQueryCurrentProcessor( ), KPF_PCID_ENABLED ) ) {

        InvpcidDescriptor.Address = Address;
        InvpcidDescriptor.Pcid = __readcr3( ) & 0xFFF;
        _invpcid( 0, &InvpcidDescriptor );
    }
    else {

        KeGenericCallIpi( MmIpiAddressFlush, Address );
    }
}
