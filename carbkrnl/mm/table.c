


#include <carbsup.h>
#include "mi.h"

//ULONG64 MmPageMapReferenceTable = 0xFFFFFF0000000000;

PPMLE
MmAddressPageTableByIndex(
    _In_ ULONG64 Level4,
    _In_ ULONG64 Level3,
    _In_ ULONG64 Level2
)
{
    BOOLEAN Zeroed;
    ULONG64 Physical;

    if ( !MiLevel4Table[ Level4 ].Present ) {
        Physical = MmAllocateZeroedPhysical( MmTypePageTable, &Zeroed );
        MiLevel4Table[ Level4 ].PageFrameNumber = Physical >> 12;
        MiLevel4Table[ Level4 ].Present = 1;
        MiLevel4Table[ Level4 ].Write = 1;
        if ( !Zeroed ) {
            RtlZeroMemory( MiReferenceLevel4Entry( Level4 ), 0x1000 );
        }
    }

    if ( !MiReferenceLevel4Entry( Level4 )[ Level3 ].Present ) {
        Physical = MmAllocateZeroedPhysical( MmTypePageTable, &Zeroed );
        MiReferenceLevel4Entry( Level4 )[ Level3 ].PageFrameNumber = Physical >> 12;
        MiReferenceLevel4Entry( Level4 )[ Level3 ].Present = 1;
        MiReferenceLevel4Entry( Level4 )[ Level3 ].Write = 1;
        if ( !Zeroed ) {
            RtlZeroMemory( MiReferenceLevel3Entry( Level4, Level3 ), 0x1000 );
        }
    }

    if ( !MiReferenceLevel3Entry( Level4, Level3 )[ Level2 ].Present ) {
        Physical = MmAllocateZeroedPhysical( MmTypePageTable, &Zeroed );
        MiReferenceLevel3Entry( Level4, Level3 )[ Level2 ].PageFrameNumber = Physical >> 12;
        MiReferenceLevel3Entry( Level4, Level3 )[ Level2 ].Present = 1;
        MiReferenceLevel3Entry( Level4, Level3 )[ Level2 ].Write = 1;
        if ( !Zeroed ) {
            RtlZeroMemory( MiReferenceLevel2Entry( Level4, Level3, Level2 ), 0x1000 );
        }
    }

    return MiReferenceLevel2Entry( Level4, Level3, Level2 );
}

PPMLE
MmAddressPageTable(
    _In_ ULONG64 Address
)
{
    BOOLEAN Zeroed;
    ULONG64 Physical;
    ULONG64 Level4;
    ULONG64 Level3;
    ULONG64 Level2;

    Level4 = MiIndexLevel4( Address );
    Level3 = MiIndexLevel3( Address );
    Level2 = MiIndexLevel2( Address );

    if ( !MiLevel4Table[ Level4 ].Present ) {
        Physical = MmAllocateZeroedPhysical( MmTypePageTable, &Zeroed );
        MiLevel4Table[ Level4 ].PageFrameNumber = Physical >> 12;
        MiLevel4Table[ Level4 ].Present = 1;
        MiLevel4Table[ Level4 ].Write = 1;
        if ( Level4 < 256 ) {
            MiLevel4Table[ Level4 ].User = 1;
        }
        if ( !Zeroed ) {
            RtlZeroMemory( MiReferenceLevel4Entry( Level4 ), 0x1000 );
        }
    }

    if ( !MiReferenceLevel4Entry( Level4 )[ Level3 ].Present ) {
        Physical = MmAllocateZeroedPhysical( MmTypePageTable, &Zeroed );
        MiReferenceLevel4Entry( Level4 )[ Level3 ].PageFrameNumber = Physical >> 12;
        MiReferenceLevel4Entry( Level4 )[ Level3 ].Present = 1;
        MiReferenceLevel4Entry( Level4 )[ Level3 ].Write = 1;
        if ( Level4 < 256 ) {
            MiReferenceLevel4Entry( Level4 )[ Level3 ].User = 1;
        }
        if ( !Zeroed ) {
            RtlZeroMemory( MiReferenceLevel3Entry( Level4, Level3 ), 0x1000 );
        }
    }

    if ( !MiReferenceLevel3Entry( Level4, Level3 )[ Level2 ].Present ) {
        Physical = MmAllocateZeroedPhysical( MmTypePageTable, &Zeroed );
        MiReferenceLevel3Entry( Level4, Level3 )[ Level2 ].PageFrameNumber = Physical >> 12;
        MiReferenceLevel3Entry( Level4, Level3 )[ Level2 ].Present = 1;
        MiReferenceLevel3Entry( Level4, Level3 )[ Level2 ].Write = 1;
        if ( Level4 < 256 ) {
            MiReferenceLevel3Entry( Level4, Level3 )[ Level2 ].User = 1;
        }
        if ( !Zeroed ) {
            RtlZeroMemory( MiReferenceLevel2Entry( Level4, Level3, Level2 ), 0x1000 );
        }
    }

    return MiReferenceLevel2Entry( Level4, Level3, Level2 );
}
