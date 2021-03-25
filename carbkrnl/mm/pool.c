


#include <carbsup.h>
#include "mi.h"

KSPIN_LOCK MmNonPagedPoolLock = { 0 };

ULONG64 MmNonPagedPoolStart = 0xFFFFFA8000000000;
ULONG64 MmNonPagedPoolEnd = 0xFFFFFD8000000000;

ULONG64 MmNonPagedPoolHint = 0xFFFFFA8000000000;

MM_POOL_HEAD MmNonPagedPoolHead = { 0 };

PVOID
MiFindFreeNonPagedPoolRegion(
    _In_ ULONG64 PageLength
)
{
    BOOLEAN SecondChance;
    ULONG64 FoundLength;
    PVOID FoundAddress;
    ULONG64 Level4;
    ULONG64 Level3;
    ULONG64 Level2;
    ULONG64 Level1;

    SecondChance = FALSE;
    FoundAddress = 0;
    FoundLength = PageLength;

    Level4 = MiIndexLevel4( MmNonPagedPoolHint );
    Level3 = MiIndexLevel3( MmNonPagedPoolHint );
    Level2 = MiIndexLevel2( MmNonPagedPoolHint );
    Level1 = MiIndexLevel1( MmNonPagedPoolHint );

SecondPass:;
    for ( ; Level4 < PAGE_MAP_INDEX_NON_PAGED_POOL_END; Level4++ ) {

        if ( MiLevel4Table[ Level4 ].Present ) {
            for ( ; Level3 < 512; Level3++ ) {

                if ( MiReferenceLevel4Entry( Level4 )[ Level3 ].Present ) {
                    for ( ; Level2 < 512; Level2++ ) {

                        if ( MiReferenceLevel3Entry( Level4, Level3 )[ Level2 ].Present ) {
                            for ( ; Level1 < 512; Level1++ ) {

                                if ( MiReferenceLevel2Entry( Level4, Level3, Level2 )[ Level1 ].Present ) {

                                    FoundLength = PageLength;
                                    FoundAddress = 0;
                                }
                                else {
                                    if ( FoundAddress == 0 ) {
                                        FoundAddress = MiConstructAddress( Level4, Level3, Level2, Level1 );
                                    }

                                    FoundLength--;
                                    if ( FoundLength == 0 ) {
                                        MmNonPagedPoolHint = ( ULONG64 )FoundAddress + ( PageLength << 12 );
                                        return FoundAddress;
                                    }
                                }
                            }
                            Level1 = 0;
                        }
                        else {
                            if ( FoundAddress == 0 ) {
                                FoundAddress = MiConstructAddress( Level4, Level3, Level2, 0 );
                            }

                            if ( FoundLength <= 512 ) {
                                MmNonPagedPoolHint = ( ULONG64 )FoundAddress + ( PageLength << 12 );
                                return FoundAddress;
                            }
                            else {
                                FoundLength -= 512;
                            }
                        }
                    }
                    Level2 = 0;
                }
                else {
                    if ( FoundAddress == 0 ) {
                        FoundAddress = MiConstructAddress( Level4, Level3, 0, 0 );
                    }

                    if ( FoundLength <= 512 * 512 ) {
                        MmNonPagedPoolHint = ( ULONG64 )FoundAddress + ( PageLength << 12 );
                        return FoundAddress;
                    }
                    else {
                        FoundLength -= 512 * 512;
                    }
                }
            }
            Level3 = 0;
        }
        else {
            if ( FoundAddress == 0 ) {
                FoundAddress = MiConstructAddress( Level4, 0, 0, 0 );
            }

            if ( FoundLength <= 512 * 512 * 512 ) {
                MmNonPagedPoolHint = ( ULONG64 )FoundAddress + ( PageLength << 12 );
                return FoundAddress;
            }
            else {
                FoundLength -= 512 * 512 * 512;
            }
        }
    }

    Level4 = PAGE_MAP_INDEX_NON_PAGED_POOL_START;
    Level3 = 0;
    Level2 = 0;
    Level1 = 0;

    if ( !SecondChance ) {
        SecondChance = TRUE;
        goto SecondPass;
    }

    KeBugCheck( STATUS_MEMORY_MANAGER );
}

PMM_POOL_TABLE_ALLOCATED_PAGES
MiCreatePoolTableAllocatedPages(

)
{
    ULONG64 Physical;
    PPMLE PageTable;
    ULONG64 PageAddress;
    PMM_POOL_TABLE_ALLOCATED_PAGES Table;

    PageAddress = ( ULONG64 )MiFindFreeNonPagedPoolRegion( 1 );
    PageTable = MmAddressPageTable( PageAddress );
    Physical = MmAllocatePhysical( MmTypeNonPagedPool );

    PageTable[ MiIndexLevel1( PageAddress ) ].PageFrameNumber = Physical >> 12;
    PageTable[ MiIndexLevel1( PageAddress ) ].Present = 1;
    PageTable[ MiIndexLevel1( PageAddress ) ].Write = 1;
    RtlZeroMemory( ( PVOID )PageAddress, 0x1000 );

    Table = ( PMM_POOL_TABLE_ALLOCATED_PAGES )PageAddress;
    Table->Usage = 0;

#if 0
    Table->Usage = 1;
    Table->Entries[ 0 ].Tag = 'LOOP';
    Table->Entries[ 0 ].Address = PageAddress;
    Table->Entries[ 0 ].Length = 0x1000;
    Table->Entries[ 0 ].Type = NonPagedPool;
#endif

    if ( MmNonPagedPoolHead.AllocatedTableCount > 0 ) {
        KeInsertEntryHead( MmNonPagedPoolHead.AllocatedTableLinks, &Table->Header.PoolLinks );
    }
    else {
        KeInitializeListHead( &Table->Header.PoolLinks );
        MmNonPagedPoolHead.AllocatedTableLinks = &Table->Header.PoolLinks;
    }
    MmNonPagedPoolHead.AllocatedTableCount++;

    return Table;
}

PMM_ALLOCATED_PAGE_ENTRY
MiFindFreeAllocatedEntry(

)
{
    PMM_POOL_TABLE_ALLOCATED_PAGES AllocatedPages;
    PLIST_ENTRY Flink;
    ULONG32 CurrentPage;

    if ( MmNonPagedPoolHead.AllocatedTableCount > 0 ) {

        Flink = MmNonPagedPoolHead.AllocatedTableLinks;
        do {
            AllocatedPages = CONTAINING_RECORD( Flink, MM_POOL_TABLE_ALLOCATED_PAGES, Header.PoolLinks );

            if ( AllocatedPages->Usage < 169 ) {

                for ( CurrentPage = 0; CurrentPage < 169; CurrentPage++ ) {

                    if ( AllocatedPages->Entries[ CurrentPage ].Type == UnusedPool ) {
                        AllocatedPages->Entries[ CurrentPage ].Type = -1;
                        AllocatedPages->Usage++;
                        return &AllocatedPages->Entries[ CurrentPage ];
                    }
                }
            }

            Flink = Flink->Flink;
        } while ( Flink != MmNonPagedPoolHead.AllocatedTableLinks );
    }

    AllocatedPages = MiCreatePoolTableAllocatedPages( );
    AllocatedPages->Entries[ AllocatedPages->Usage ].Type = -1;

    return &AllocatedPages->Entries[ AllocatedPages->Usage++ ];
}

PMM_POOL_TABLE_ALLOCATED_PAGES
MiFindOccupiedAllocatedEntry(
    _In_  PVOID    Address,
    _In_  ULONG32  Tag,
    _Out_ PULONG32 PageIndex
)
{
    PMM_POOL_TABLE_ALLOCATED_PAGES AllocatedPages;
    PLIST_ENTRY Flink;
    ULONG32 CurrentPage;

    if ( MmNonPagedPoolHead.AllocatedTableCount > 0 ) {

        Flink = MmNonPagedPoolHead.AllocatedTableLinks;
        do {
            AllocatedPages = CONTAINING_RECORD( Flink, MM_POOL_TABLE_ALLOCATED_PAGES, Header.PoolLinks );

            for ( CurrentPage = 0; CurrentPage < 169; CurrentPage++ ) {

                if ( AllocatedPages->Entries[ CurrentPage ].Type != UnusedPool &&
                     AllocatedPages->Entries[ CurrentPage ].Address == ( ULONG64 )Address &&
                     AllocatedPages->Entries[ CurrentPage ].Tag == Tag ) {

                    *PageIndex = CurrentPage;
                    return AllocatedPages;
                }
            }

            Flink = Flink->Flink;
        } while ( Flink != MmNonPagedPoolHead.AllocatedTableLinks );
    }

    *PageIndex = 0;
    return NULL;
}

PMM_POOL_TABLE_GRANULAR_PAGES
MiCreatePoolTableGranularPages(

)
{
    ULONG64 Physical;
    PPMLE PageTable;
    ULONG64 PageAddress;
    BOOLEAN Zeroed;
    PMM_POOL_TABLE_GRANULAR_PAGES Table;

    PageAddress = ( ULONG64 )MiFindFreeNonPagedPoolRegion( 1 );
    PageTable = MmAddressPageTable( PageAddress );
    Physical = MmAllocateZeroedPhysical( MmTypeNonPagedPool, &Zeroed );

    PageTable[ MiIndexLevel1( PageAddress ) ].PageFrameNumber = Physical >> 12;
    PageTable[ MiIndexLevel1( PageAddress ) ].Present = 1;
    PageTable[ MiIndexLevel1( PageAddress ) ].Write = 1;
    if ( !Zeroed ) {
        RtlZeroMemory( ( PVOID )PageAddress, 0x1000 );
    }

    Table = ( PMM_POOL_TABLE_GRANULAR_PAGES )PageAddress;
    Table->Usage = 0;

#if 0
    Entry = MiFindFreeAllocatedEntry( );
    Entry->Type = NonPagedPool;
    Entry->Tag = 'LOOP';
    Entry->Length = 0x1000;
    Entry->Address = Physical;
#endif

    if ( MmNonPagedPoolHead.GranularTableCount > 0 ) {

        KeInsertEntryHead( MmNonPagedPoolHead.GranularTableLinks, &Table->Header.PoolLinks );
    }
    else {

        KeInitializeListHead( &Table->Header.PoolLinks );
        MmNonPagedPoolHead.GranularTableLinks = &Table->Header.PoolLinks;
    }
    MmNonPagedPoolHead.GranularTableCount++;

    return ( PVOID )PageAddress;
}

PVOID
MiFindFreeGranularPage(
    _In_ POOL_TYPE Type,
    _In_ ULONG64   GranularLength
)
{
    PMM_POOL_TABLE_GRANULAR_PAGES GranularPages;
    PLIST_ENTRY Flink;
    ULONG64 CurrentPage;
    ULONG64 RequiredMask;
    ULONG64 RequiredLength;
    ULONG64 Physical;
    PPMLE PageTable;
    ULONG64 PageAddress;
    BOOLEAN Zeroed;
    ULONG64 CurrentIndex;

    RequiredLength = GranularLength;
    RequiredMask = 0;

    for ( int i = 0; i < RequiredLength; i++ ) {

        RequiredMask |= 1ULL << i;
    }

    if ( MmNonPagedPoolHead.GranularTableCount > 0 ) {

        Flink = MmNonPagedPoolHead.GranularTableLinks;
        do {
            GranularPages = CONTAINING_RECORD( Flink, MM_POOL_TABLE_GRANULAR_PAGES, Header.PoolLinks );

            if ( GranularPages->Usage < 169 ) {

                for ( CurrentPage = 0; CurrentPage < 169; CurrentPage++ ) {

                    if ( GranularPages->Entries[ CurrentPage ].Type == UnusedPool ) {

                        PageAddress = ( ULONG64 )MiFindFreeNonPagedPoolRegion( 1 );
                        PageTable = MmAddressPageTable( PageAddress );
                        Physical = MmAllocateZeroedPhysical( MmTypeNonPagedPool, &Zeroed );
                        PageTable[ MiIndexLevel1( PageAddress ) ].PageFrameNumber = Physical >> 12;
                        PageTable[ MiIndexLevel1( PageAddress ) ].Present = 1;
                        PageTable[ MiIndexLevel1( PageAddress ) ].Write = 1;
                        PageTable[ MiIndexLevel1( PageAddress ) ].ExecuteDisable = Type == NonPagedPoolZeroed;
                        if ( !Zeroed ) {
                            RtlZeroMemory( ( PVOID )PageAddress, 0x1000 );
                        }
                        GranularPages->Entries[ CurrentPage ].Page = PageAddress;
                        GranularPages->Entries[ CurrentPage ].Bitmap = RequiredMask;
                        GranularPages->Entries[ CurrentPage ].Type = Type;
                        GranularPages->Usage++;

                        return ( PVOID )( GranularPages->Entries[ CurrentPage ].Page );
                    }

                    if ( GranularPages->Entries[ CurrentPage ].Type != Type ) {
                        continue;
                    }

                    for ( CurrentIndex = 0; CurrentIndex < 64 - RequiredLength; CurrentIndex++ ) {
                        if ( ( GranularPages->Entries[ CurrentPage ].Bitmap & ( RequiredMask << CurrentIndex ) ) == 0 ) {

                            GranularPages->Entries[ CurrentPage ].Bitmap |= RequiredMask << CurrentIndex;
                            GranularPages->Usage++;
                            return ( PVOID )( GranularPages->Entries[ CurrentPage ].Page + ( CurrentIndex << 6 ) );
                        }
                    }
                }
            }

            Flink = Flink->Flink;
        } while ( Flink != MmNonPagedPoolHead.GranularTableLinks );
    }

    GranularPages = MiCreatePoolTableGranularPages( );
    PageAddress = ( ULONG64 )MiFindFreeNonPagedPoolRegion( 1 );
    PageTable = MmAddressPageTable( PageAddress );
    Physical = MmAllocateZeroedPhysical( MmTypeNonPagedPool, &Zeroed );
    PageTable[ MiIndexLevel1( PageAddress ) ].PageFrameNumber = Physical >> 12;
    PageTable[ MiIndexLevel1( PageAddress ) ].Present = 1;
    PageTable[ MiIndexLevel1( PageAddress ) ].Write = 1;
    PageTable[ MiIndexLevel1( PageAddress ) ].ExecuteDisable = Type == NonPagedPoolZeroed;
    if ( !Zeroed ) {
        RtlZeroMemory( ( PVOID )PageAddress, 0x1000 );
    }
    GranularPages->Entries[ GranularPages->Usage ].Page = PageAddress;
    GranularPages->Entries[ GranularPages->Usage ].Bitmap = RequiredMask;
    GranularPages->Entries[ GranularPages->Usage ].Type = Type;

    return ( PVOID )GranularPages->Entries[ GranularPages->Usage++ ].Page;
}

PMM_POOL_TABLE_GRANULAR_PAGES
MiFindOccupiedGranularEntry(
    _In_  PVOID    Address,
    _Out_ PULONG32 PageIndex
)
{
    ULONG64 PageAddress;

    PageAddress = ( ULONG64 )Address & ~0xFFF;

    PMM_POOL_TABLE_GRANULAR_PAGES GranularPages;
    PLIST_ENTRY Flink;
    ULONG32 CurrentPage;

    if ( MmNonPagedPoolHead.GranularTableCount > 0 ) {

        Flink = MmNonPagedPoolHead.GranularTableLinks;
        do {
            GranularPages = CONTAINING_RECORD( Flink, MM_POOL_TABLE_GRANULAR_PAGES, Header.PoolLinks );

            for ( CurrentPage = 0; CurrentPage < 169; CurrentPage++ ) {

                if ( GranularPages->Entries[ CurrentPage ].Page == PageAddress ) {

                    *PageIndex = CurrentPage;
                    return GranularPages;
                }
            }

            Flink = Flink->Flink;
        } while ( Flink != MmNonPagedPoolHead.GranularTableLinks );
    }

    *PageIndex = 0;
    return NULL;
}

PVOID
MmMapIoSpace(
    _In_ ULONG64 Physical,
    _In_ ULONG64 Length
)
{
    return MmMapIoSpaceSpecifyCache( Physical,
                                     Length,
                                     MmCacheUncacheable );
}

PVOID
MmMapIoSpaceSpecifyCache(
    _In_ ULONG64       Physical,
    _In_ ULONG64       Length,
    _In_ MM_CACHE_TYPE Cache
)
{
    PMM_ALLOCATED_PAGE_ENTRY Allocated;
    ULONG64 PageLength;
    ULONG64 PageAddress;
    ULONG64 PageOffset;
    ULONG64 CurrentPage;
    PPMLE PageTable;
    KIRQL PreviousIrql;

    PageOffset = Physical & 0xFFF;
    Physical &= ~0xFFF;
    PageLength = ROUND_TO_PAGES( Length + PageOffset ) >> 12;

    KeAcquireSpinLock( &MmNonPagedPoolLock, &PreviousIrql );

    PageAddress = ( ULONG64 )MiFindFreeNonPagedPoolRegion( PageLength );

    for ( CurrentPage = 0; CurrentPage < PageLength; CurrentPage++ ) {

        PageTable = MmAddressPageTable( PageAddress + ( CurrentPage << 12 ) );

        PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].PageFrameNumber = ( Physical >> 12 ) + CurrentPage;
        PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].Present = 1;
        PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].Write = 1;
        PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].ExecuteDisable = 1;

        switch ( Cache ) {
        default:
        case MmCacheUncacheable:
            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].Pat = 0;
            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].CacheDisable = 1;
            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].WriteThrough = 0; // Pa2
            break;
        case MmCacheWriteBack:
            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].Pat = 0;
            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].CacheDisable = 0;
            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].WriteThrough = 0; // Pa0
            break;
        case MmCacheWriteCombining:
            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].Pat = 0;
            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].CacheDisable = 0;
            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].WriteThrough = 1; // Pa1
            break;
        }
    }

    Allocated = MiFindFreeAllocatedEntry( );
    Allocated->Address = PageAddress;
    Allocated->Length = Length;
    Allocated->Tag = 'OIMM';
    Allocated->Type = NonPagedPool;

    KeReleaseSpinLock( &MmNonPagedPoolLock, PreviousIrql );

    return ( PVOID )( PageAddress + PageOffset );
}

PVOID
MmAllocatePoolWithTag(
    _In_ POOL_TYPE Type,
    _In_ ULONG64   Length,
    _In_ ULONG32   Tag
)
{
    PMM_ALLOCATED_PAGE_ENTRY Allocated;
    PPMLE PageTable;
    ULONG64 PageLength;
    ULONG64 PageAddress;
    ULONG64 Physical;
    BOOLEAN Zeroed;
    KIRQL PreviousIrql;

    KeAcquireSpinLock( &MmNonPagedPoolLock, &PreviousIrql );

    Length = ROUND( Length, 0x40 );
    if ( Length < 0x1000 ) {

        if ( Type == NonPagedPool ) {

            Type = NonPagedPoolZeroed;
        }

        if ( Type == NonPagedPoolExecute ) {

            Type = NonPagedPoolZeroedExecute;
        }

        PageLength = Length >> 6;
        PageAddress = ( ULONG64 )MiFindFreeGranularPage( Type, PageLength );
    }
    else {

        Length = ROUND_TO_PAGES( Length );

        PageLength = Length >> 12;
        PageAddress = ( ULONG64 )MiFindFreeNonPagedPoolRegion( PageLength );

        while ( PageLength-- ) {

            PageTable = MmAddressPageTable( PageAddress + ( PageLength << 12 ) );

            switch ( Type ) {
            case NonPagedPoolZeroedExecute:
                Physical = MmAllocateZeroedPhysical( MmTypeNonPagedPool, &Zeroed );

                PageTable[ MiIndexLevel1( PageAddress + ( PageLength << 12 ) ) ].PageFrameNumber = ( Physical >> 12 );
                PageTable[ MiIndexLevel1( PageAddress + ( PageLength << 12 ) ) ].Present = 1;
                PageTable[ MiIndexLevel1( PageAddress + ( PageLength << 12 ) ) ].Write = 1;
                if ( !Zeroed ) {
                    RtlZeroMemory( ( PVOID )( PageAddress + ( PageLength << 12 ) ), 0x1000 );
                }
                break;
            case NonPagedPoolZeroed:
                Physical = MmAllocateZeroedPhysical( MmTypeNonPagedPool, &Zeroed );

                PageTable[ MiIndexLevel1( PageAddress + ( PageLength << 12 ) ) ].PageFrameNumber = ( Physical >> 12 );
                PageTable[ MiIndexLevel1( PageAddress + ( PageLength << 12 ) ) ].Present = 1;
                PageTable[ MiIndexLevel1( PageAddress + ( PageLength << 12 ) ) ].Write = 1;
                PageTable[ MiIndexLevel1( PageAddress + ( PageLength << 12 ) ) ].ExecuteDisable = 1;
                if ( !Zeroed ) {
                    RtlZeroMemory( ( PVOID )( PageAddress + ( PageLength << 12 ) ), 0x1000 );
                }
                break;
            case NonPagedPoolExecute:
                Physical = MmAllocatePhysical( MmTypeNonPagedPool );

                PageTable[ MiIndexLevel1( PageAddress + ( PageLength << 12 ) ) ].PageFrameNumber = ( Physical >> 12 );
                PageTable[ MiIndexLevel1( PageAddress + ( PageLength << 12 ) ) ].Present = 1;
                PageTable[ MiIndexLevel1( PageAddress + ( PageLength << 12 ) ) ].Write = 1;
                break;
            case NonPagedPool:
            default:
                Physical = MmAllocatePhysical( MmTypeNonPagedPool );

                PageTable[ MiIndexLevel1( PageAddress + ( PageLength << 12 ) ) ].PageFrameNumber = ( Physical >> 12 );
                PageTable[ MiIndexLevel1( PageAddress + ( PageLength << 12 ) ) ].Present = 1;
                PageTable[ MiIndexLevel1( PageAddress + ( PageLength << 12 ) ) ].Write = 1;
                PageTable[ MiIndexLevel1( PageAddress + ( PageLength << 12 ) ) ].ExecuteDisable = 1;
                break;
            }
        }
    }

    Allocated = MiFindFreeAllocatedEntry( );
    Allocated->Address = PageAddress;
    Allocated->Length = Length;
    Allocated->Tag = Tag;
    Allocated->Type = Type;

    KeReleaseSpinLock( &MmNonPagedPoolLock, PreviousIrql );

    return ( PVOID )PageAddress;
}

VOID
MmFreePoolWithTag(
    _In_ PVOID   Pool,
    _In_ ULONG32 Tag
)
{
    PMM_POOL_TABLE_GRANULAR_PAGES GranularPages;
    PMM_GRANULAR_PAGE_ENTRY Granular;
    PMM_POOL_TABLE_ALLOCATED_PAGES AllocatedPages;
    PMM_ALLOCATED_PAGE_ENTRY Allocated;
    ULONG64 PageAddress;
    ULONG64 PageLength;
    ULONG64 RequiredMask;
    ULONG64 RequiredLength;
    ULONG32 AllocatedEntry;
    ULONG32 GranularEntry;
    ULONG64 CurrentPage;
    PPMLE PageTable;
    KIRQL PreviousIrql;

    KeAcquireSpinLock( &MmNonPagedPoolLock, &PreviousIrql );

    AllocatedPages = MiFindOccupiedAllocatedEntry( Pool, Tag, &AllocatedEntry );

    if ( AllocatedPages == NULL ) {

        KeBugCheck( STATUS_MEMORY_MANAGER );
    }

    Allocated = &AllocatedPages->Entries[ AllocatedEntry ];

    if ( Allocated->Length < 0x1000 ) {

        GranularPages = MiFindOccupiedGranularEntry( Pool, &GranularEntry );
        Granular = &GranularPages->Entries[ GranularEntry ];

        if ( GranularPages == NULL ) {

            Allocated->Type = UnusedPool;
            AllocatedPages->Usage--;
            KeReleaseSpinLock( &MmNonPagedPoolLock, PreviousIrql );
            return;
        }

        RequiredLength = Allocated->Length >> 6;
        RequiredMask = 0;

        for ( int i = 0; i < RequiredLength; i++ ) {

            RequiredMask |= 1ULL << i;
        }

        RequiredMask <<= ( Allocated->Address & 0xFFF ) >> 6;

        Granular->Bitmap &= ~RequiredMask;
        GranularPages->Usage--;

        if ( Granular->Bitmap == 0 ) {

            PageTable = MmAddressPageTable( Granular->Page );
            MmFreePhysical( PageTable[ MiIndexLevel1( Granular->Page ) ].PageFrameNumber << 12 );
            PageTable[ MiIndexLevel1( Granular->Page ) ].Long = 0;
            Granular->Type = UnusedPool;
        }
        else {

            RtlZeroMemory( ( PVOID )Allocated->Address, ( ULONG )Allocated->Length );
        }

        Allocated->Type = UnusedPool;
        AllocatedPages->Usage--;

        KeReleaseSpinLock( &MmNonPagedPoolLock, PreviousIrql );

        if ( Granular->Type == UnusedPool ) {

            MmFlushAddress( ( PVOID )( Granular->Page ) );
        }
    }
    else {

        PageAddress = Allocated->Address;
        PageLength = ROUND_TO_PAGES( Allocated->Length ) / 0x1000;

        for ( CurrentPage = 0; CurrentPage < PageLength; CurrentPage++ ) {

            PageTable = MmAddressPageTable( PageAddress + ( CurrentPage << 12 ) );

            MmFreePhysical( PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].PageFrameNumber << 12 );
            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].Long = 0;
        }

        Allocated->Type = UnusedPool;
        AllocatedPages->Usage--;

        KeReleaseSpinLock( &MmNonPagedPoolLock, PreviousIrql );

        for ( CurrentPage = 0; CurrentPage < PageLength; CurrentPage++ ) {

            MmFlushAddress( ( PVOID )( PageAddress + ( CurrentPage << 12 ) ) );
        }
    }
}

VOID
MmUnmapIoSpace(
    _In_ PVOID Pool
)
{
    PMM_POOL_TABLE_ALLOCATED_PAGES AllocatedPages;
    PMM_ALLOCATED_PAGE_ENTRY Allocated;
    ULONG32 AllocatedEntry;
    ULONG64 PageAddress;
    ULONG64 PageLength;
    ULONG64 CurrentPage;
    PPMLE PageTable;
    KIRQL PreviousIrql;

    KeAcquireSpinLock( &MmNonPagedPoolLock, &PreviousIrql );

    ( ( ULONG64 )Pool ) &= ~0xFFF;
    AllocatedPages = MiFindOccupiedAllocatedEntry( Pool, 'OIMM', &AllocatedEntry );
    Allocated = &AllocatedPages->Entries[ AllocatedEntry ];

    if ( AllocatedPages == NULL ) {

        KeBugCheck( STATUS_MEMORY_MANAGER );
    }

    PageAddress = Allocated->Address;
    PageLength = ROUND_TO_PAGES( Allocated->Length ) / 0x1000;

    for ( CurrentPage = 0; CurrentPage < PageLength; CurrentPage++ ) {

        PageTable = MmAddressPageTable( PageAddress + ( CurrentPage << 12 ) );

        PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].Long = 0;
    }

    Allocated->Type = UnusedPool;
    AllocatedPages->Usage--;

    KeReleaseSpinLock( &MmNonPagedPoolLock, PreviousIrql );

    for ( CurrentPage = 0; CurrentPage < PageLength; CurrentPage++ ) {

        MmFlushAddress( ( PVOID )( PageAddress + ( CurrentPage << 12 ) ) );
    }
}
