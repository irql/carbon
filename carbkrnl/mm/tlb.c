


#include <carbsup.h>
#include "mi.h"
#include "../hal/halp.h"
#include "../ke/ki.h"

VOID
MmIpiAddressFlush(
    _In_ PVOID Address
)
{
    KiInvlpg( Address );
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
        InvpcidDescriptor.Pcid = MiGetAddressSpace( ) & 0xFFF;
        KiInvpcid( 0, &InvpcidDescriptor );
    }
    else {

        KeGenericCallIpi( MmIpiAddressFlush, Address );
    }
}
