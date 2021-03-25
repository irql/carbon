


#include <carbsup.h>
#include "mi.h"

ULONG64
MmAllocatePhysical(
    _In_ MM_VA_TYPE Reason
)
{
    return MmAllocatePhysicalWithPfn( Reason, NULL );
}

ULONG64
MmAllocatePhysicalWithPfn(
    _In_      MM_VA_TYPE Reason,
    _Out_opt_ PMM_PFN*   Pfn
)
{
    PMM_PFN Pfn1;

    Pfn1 = MmReferenceDatabaseByLinkedType( MmTypeModified );
    MmChangePfnVaType( Pfn1, Reason );
    Pfn1->ReferenceCount++;
    Pfn1->LockBit = FALSE;

    if ( Pfn != NULL ) {
        *Pfn = Pfn1;
    }

    return Pfn1->PageFrameNumber << 12;
}

ULONG64
MmAllocateZeroedPhysical(
    _In_  MM_VA_TYPE Reason,
    _Out_ PBOOLEAN   Zeroed
)
{
    return MmAllocateZeroedPhysicalWithPfn( Reason, Zeroed, NULL );
}

ULONG64
MmAllocateZeroedPhysicalWithPfn(
    _In_      MM_VA_TYPE Reason,
    _Out_     PBOOLEAN   Zeroed,
    _Out_opt_ PMM_PFN*   Pfn
)
{
    PMM_PFN Pfn1;

    *Zeroed = MmZeroedPageListHead.Total != 0;
    if ( MmZeroedPageListHead.Total != 0 ) {
        Pfn1 = MmReferenceDatabaseByLinkedType( MmTypeZeroed );
        MmChangePfnVaType( Pfn1, Reason );
        Pfn1->ReferenceCount++;
        Pfn1->LockBit = FALSE;

        if ( Pfn != NULL ) {
            *Pfn = Pfn1;
        }

        return Pfn1->PageFrameNumber << 12;
    }
    else {

        return MmAllocatePhysicalWithPfn( Reason, Pfn );
    }

}

VOID
MmFreePhysical(
    _In_ ULONG64 Address
)
{
    PMM_PFN Pfn;

    Pfn = MmReferenceDatabaseByAddress( Address );
    Pfn->ReferenceCount--;
    if ( Pfn->ReferenceCount == 0 ) {
        MmChangePfnVaType( Pfn, MmTypeModified );
    }

    Pfn->LockBit = FALSE;
}
