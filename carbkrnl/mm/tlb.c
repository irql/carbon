


#include <carbsup.h>
#include "mi.h"
#include "../hal/halp.h"
#include "../ke/ki.h"

#if 1

VOID
MmFlushAddressDpc(
    _In_ PVOID Address
)
{
    __invlpg( Address );
}

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
    KeGenericCallIpi( MmIpiAddressFlush, Address );
}

#else

VOID
MmFlushAddress(
    _In_ PVOID Address
)
{
    //
    // Invpcid is pog. We use type 3 for complete pcid invalidation.
    //
#if 0
    struct {
        PVOID   Address;
        ULONG64 Pcid;
    } InvpcidDescriptor;

    InvpcidDescriptor.Address = Address;
    InvpcidDescriptor.Pcid = 0;
#endif
    __m128i InvpcidDescriptor;
    InvpcidDescriptor.m128i_i64[ 0 ] = 0;
    InvpcidDescriptor.m128i_i64[ 1 ] = ( long long )Address;

    _invpcid( 0, &InvpcidDescriptor );
}

#endif
