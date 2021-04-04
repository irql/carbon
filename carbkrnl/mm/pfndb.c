


#include <carbsup.h>
#include "mi.h"

//
// All of these API's can be optimized
// and should be optimized in the future.
//

ULONG64 MmPfnCount = 0;
PMM_PFN MmPfnDatabase = ( PMM_PFN )( 0xFFFFFE8000000000 );

MM_PFN_LIST MmZeroedPageListHead = { 0 };
MM_PFN_LIST MmModifiedPageListHead = { 0 };

VOID
MmChangePfnVaType(
    _In_ PMM_PFN   Pfn,
    _In_ MM_VA_TYPE Type
)
{

    if ( Pfn->VaType == MmTypeZeroed &&
         Pfn->VaType != Type ) {
        //RtlDebugPrint( L"unlinked zeroed page\n" );

        MmRemovePfnListEntry( &MmZeroedPageListHead, Pfn );
    }

    if ( Pfn->VaType == MmTypeModified &&
         Pfn->VaType != Type ) {
        //RtlDebugPrint( L"unlinked modified page\n" );

        MmRemovePfnListEntry( &MmModifiedPageListHead, Pfn );
    }

    if ( Type == MmTypeModified &&
         Type != Pfn->VaType ) {
        //RtlDebugPrint( L"linked modified page\n" );

        MmInsertPfnListEntry( &MmModifiedPageListHead, Pfn );
    }

    if ( Type == MmTypeZeroed &&
         Type != Pfn->VaType ) {
        //RtlDebugPrint( L"linked zero page\n" );

        MmInsertPfnListEntry( &MmZeroedPageListHead, Pfn );
    }

    Pfn->VaType = Type;
}

VOID
MmChangeDatabaseMemoryVaType(
    _In_ ULONG64    Address,
    _In_ ULONG64    Length,
    _In_ MM_VA_TYPE Type
)
{
    ULONG64 CurrentPfn;
    ULONG64 MinimumPfn;
    ULONG64 MaximumPfn;

    CurrentPfn = 0;
    MinimumPfn = Address >> 12;
    MaximumPfn = MinimumPfn + ( Length >> 12 );

    //
    // This cycles the database one time, and does no error checking.
    // It will search for any pfn's in the specified range and change
    // their VaType, if found.
    //

    while ( CurrentPfn < MmPfnCount ) {
        if ( MmPfnDatabase[ CurrentPfn ].PageFrameNumber < MinimumPfn ||
             MmPfnDatabase[ CurrentPfn ].PageFrameNumber > MaximumPfn ) {
            CurrentPfn++;
            continue;
        }

        MmChangePfnVaType( &MmPfnDatabase[ CurrentPfn ], Type );
        CurrentPfn++;
    }
}

PMM_PFN
MmReferenceDatabaseByAddress(
    _In_ ULONG64 Address
)
{
    ULONG64 CurrentPfn;

    CurrentPfn = 0;

    while ( CurrentPfn < MmPfnCount ) {
        if ( MmPfnDatabase[ CurrentPfn ].PageFrameNumber == Address >> 12 ) {

            //while ( MmPfnDatabase[ CurrentPfn ].LockBit )
            //    ;

            MmPfnDatabase[ CurrentPfn ].LockBit = TRUE;
            MmPfnDatabase[ CurrentPfn ].ReferenceCount++;

            return &MmPfnDatabase[ CurrentPfn ];
        }
        CurrentPfn++;
    }
    return NULL;
}

PMM_PFN
MmReferenceDatabaseByType(
    _In_ MM_VA_TYPE Type
)
{
    // unreferenced useless function - could be removed
    // has an issue with lock bit

    ULONG64 CurrentPfn;

    CurrentPfn = 0;

    while ( CurrentPfn < MmPfnCount ) {
        if ( MmPfnDatabase[ CurrentPfn ].VaType == Type ) {

            MmPfnDatabase[ CurrentPfn ].LockBit = TRUE;
            MmPfnDatabase[ CurrentPfn ].ReferenceCount++;

            return &MmPfnDatabase[ CurrentPfn ];
        }
        CurrentPfn++;
    }
    return NULL;
}

PMM_PFN
MmReferenceDatabaseByLinkedType(
    _In_ MM_VA_TYPE Type
)
{
    PMM_PFN Pfn;
    PLIST_ENTRY Flink;

    if ( Type == MmTypeModified ) {
        if ( MmModifiedPageListHead.Total == 0 ) {
            return MmReferenceDatabaseByLinkedType( MmTypeZeroed );
        }

        Flink = MmModifiedPageListHead.Head;
        do {
            Pfn = CONTAINING_RECORD( Flink, MM_PFN, PfnLinks );

            Flink = Flink->Flink;
        } while ( Pfn->LockBit );

        Pfn->LockBit = TRUE;

        return Pfn;
    }
    else if ( Type == MmTypeZeroed ) {
        if ( MmZeroedPageListHead.Total < MmZeroedPageListHead.Quota ) {

            if ( MiZeroQuotaEvent ) {
                KeSignalEvent( MiZeroQuotaEvent, TRUE );
            }
        }

        if ( MmZeroedPageListHead.Total == 0 ) {
            return NULL;
        }

        Flink = MmZeroedPageListHead.Head;
        do {
            Pfn = CONTAINING_RECORD( Flink, MM_PFN, PfnLinks );

            Flink = Flink->Flink;
        } while ( Pfn->LockBit );

        Pfn->LockBit = TRUE;

        return Pfn;
    }
    else {
        return NULL;
    }
}

VOID
MmMatchQuotaZeroListEvent(

)
{
    //
    // This procedure should be created as a thread
    // waiting for an event to be signaled, the event
    // being that the ZeroList total has dropped below
    // the quota, this should move MmTypeModified pages
    // into the MmTypeZeroed list after zeroing the amount
    // demanded. 
    //

    PMM_PFN Pfn;
    PVOID Page;
    KIRQL PreviousIrql;

    while ( MmZeroedPageListHead.Quota > MmZeroedPageListHead.Total ) {

        KeAcquireSpinLock( &MmNonPagedPoolLock, &PreviousIrql );
        Pfn = MmReferenceDatabaseByLinkedType( MmTypeModified );
        KeReleaseSpinLock( &MmNonPagedPoolLock, PreviousIrql );

        if ( Pfn == NULL ) {

            break;
        }

        MmChangePfnVaType( Pfn, MmTypeSystemLocked );

        Page = MmMapIoSpace( Pfn->PageFrameNumber << 12, 0x1000 );
        RtlZeroMemory( Page, 0x1000 );
        MmUnmapIoSpace( Page );

        MmChangePfnVaType( Pfn, MmTypeZeroed );
        Pfn->LockBit = FALSE;
    }

}
