


#include <carbsup.h>
#include "../hal/halp.h"
#include "mi.h"
#include "../ke/ki.h"

PKEVENT MiZeroQuotaEvent;

VOID
MmInitializeMemoryManager(
    _In_ PMEMORY_MAP PhysicalMap
)
{

    ULONG64 DatabaseLength;
    ULONG64 DatabasePhysical;
    ULONG64 OriginalLength;
    ULONG64 OriginalPhysical;
    ULONG32 Entry;
    ULONG64 KernelEnd;
    ULONG32 PageMapUsage;
    ULONG32 CurrentPage;
    ULONG64 CurrentPfn;

    ULONG64 Base;
    ULONG64 Length;

    PIMAGE_DOS_HEADER KernelDos;
    PIMAGE_NT_HEADERS KernelNt;
    PIMAGE_SECTION_HEADER KernelLastSection;

    DatabasePhysical = 0;
    PageMapUsage = 0;

    for ( Entry = 0; Entry < PhysicalMap->EntryCount; Entry++ ) {

        if ( PhysicalMap->Entries[ Entry ].RegionType == 1 ) {

            PhysicalMap->Entries[ Entry ].RegionBase &= ~0xFFF;
            PhysicalMap->Entries[ Entry ].RegionLength &= ~0xFFF;

            if ( PhysicalMap->Entries[ Entry ].RegionLength >= 0x1000 ) {

                MmPfnCount += PhysicalMap->Entries[ Entry ].RegionLength >> 12;
            }
        }
    }

    DatabaseLength = ROUND_TO_PAGES( MmPfnCount * sizeof( MM_PFN ) );

    //
    // This is an accumulation of all required
    // physical memory for the PFN database, it
    // also accounts for page table memory which
    // is what these calculations are doing.
    //
    // PageMapUsage is a variable which has the upper
    // 16 bits be the usage of level 3 and lower 16
    // as the usage of level 2
    //

    PageMapUsage |= MiIndexLevel3( DatabaseLength ) == 0 ? ( 1 << 16 ) : ( MiIndexLevel3( ROUND( DatabaseLength, 0x40000000 ) ) << 16 );
    PageMapUsage |= MiIndexLevel2( DatabaseLength ) == 0 ? ( 1 ) : ( MiIndexLevel2( ROUND( DatabaseLength, 0x200000 ) ) );

    DatabaseLength += 0x1000;
    DatabaseLength += ( ( ULONG64 )PageMapUsage >> 16 ) << 12;
    DatabaseLength += ( ( ULONG64 )PageMapUsage & 0xFFFF ) << 12;

    KernelDos = ( PIMAGE_DOS_HEADER )( 0xFFFFFFFFFFE00000 );
    KernelNt = ( PIMAGE_NT_HEADERS )( 0xFFFFFFFFFFE00000 + KernelDos->e_lfanew );
    KernelLastSection = &IMAGE_FIRST_SECTION( KernelNt )[ KernelNt->FileHeader.NumberOfSections - 1 ];

    KernelEnd = 0x200000ull + ( ULONG64 )KernelLastSection->VirtualAddress + ROUND_TO_PAGES( KernelLastSection->Misc.VirtualSize );

    for ( Entry = 0; Entry < PhysicalMap->EntryCount; Entry++ ) {

        if ( PhysicalMap->Entries[ Entry ].RegionType == 1 &&
             PhysicalMap->Entries[ Entry ].RegionLength >= DatabaseLength &&
             PhysicalMap->Entries[ Entry ].RegionBase + PhysicalMap->Entries[ Entry ].RegionLength >= KernelEnd ) {
            Length = PhysicalMap->Entries[ Entry ].RegionLength;
            Base = PhysicalMap->Entries[ Entry ].RegionBase;

            while ( Length > DatabaseLength && Base <= KernelEnd ) {
                Base += 0x1000;
                Length -= 0x1000;
            }

            if ( Length > DatabaseLength ) {
                DatabasePhysical = Base;
                break;
            }
            else {
                DatabasePhysical = 0;
            }
        }
    }

    if ( DatabasePhysical == 0 ) {

        return;
    }

    OriginalPhysical = DatabasePhysical;
    OriginalLength = DatabaseLength;

    ( ( PPMLE )__readcr3( ) )[ PAGE_MAP_INDEX_PFN_DATABASE ].PageFrameNumber = DatabasePhysical >> 12;
    ( ( PPMLE )__readcr3( ) )[ PAGE_MAP_INDEX_PFN_DATABASE ].Present = 1;
    ( ( PPMLE )__readcr3( ) )[ PAGE_MAP_INDEX_PFN_DATABASE ].Write = 1;
    RtlZeroMemory( MiReferenceLevel4Entry( PAGE_MAP_INDEX_PFN_DATABASE ), 0x1000 );
    DatabasePhysical += 0x1000;
    DatabaseLength -= 0x1000;

    for ( CurrentPage = 0; CurrentPage < ( PageMapUsage >> 16 & 0xFFFF ); CurrentPage++ ) {
        MiReferenceLevel4Entry( PAGE_MAP_INDEX_PFN_DATABASE )[ CurrentPage ].PageFrameNumber = DatabasePhysical >> 12;
        MiReferenceLevel4Entry( PAGE_MAP_INDEX_PFN_DATABASE )[ CurrentPage ].Present = 1;
        MiReferenceLevel4Entry( PAGE_MAP_INDEX_PFN_DATABASE )[ CurrentPage ].Write = 1;
        RtlZeroMemory( MiReferenceLevel3Entry( PAGE_MAP_INDEX_PFN_DATABASE, CurrentPage ), 0x1000 );
        DatabasePhysical += 0x1000;
        DatabaseLength -= 0x1000;
    }

    for ( CurrentPage = 0; CurrentPage < ( PageMapUsage & 0xFFFF ); CurrentPage++ ) {
        MiReferenceLevel3Entry( PAGE_MAP_INDEX_PFN_DATABASE, CurrentPage / 0x200 )[ CurrentPage ].PageFrameNumber = DatabasePhysical >> 12;
        MiReferenceLevel3Entry( PAGE_MAP_INDEX_PFN_DATABASE, CurrentPage / 0x200 )[ CurrentPage ].Present = 1;
        MiReferenceLevel3Entry( PAGE_MAP_INDEX_PFN_DATABASE, CurrentPage / 0x200 )[ CurrentPage ].Write = 1;
        RtlZeroMemory( MiReferenceLevel2Entry( PAGE_MAP_INDEX_PFN_DATABASE, CurrentPage / 0x200, CurrentPage ), 0x1000 );
        DatabasePhysical += 0x1000;
        DatabaseLength -= 0x1000;
    }

    for ( CurrentPage = 0; CurrentPage < DatabaseLength / 0x1000; CurrentPage++ ) {
        MiReferenceLevel2Entry( PAGE_MAP_INDEX_PFN_DATABASE, CurrentPage / 0x200000, CurrentPage / 0x200 )[ CurrentPage % 0x200 ].PageFrameNumber = DatabasePhysical >> 12;
        MiReferenceLevel2Entry( PAGE_MAP_INDEX_PFN_DATABASE, CurrentPage / 0x200000, CurrentPage / 0x200 )[ CurrentPage % 0x200 ].Present = 1;
        MiReferenceLevel2Entry( PAGE_MAP_INDEX_PFN_DATABASE, CurrentPage / 0x200000, CurrentPage / 0x200 )[ CurrentPage % 0x200 ].Write = 1;
        DatabasePhysical += 0x1000;
    }

    MmModifiedPageListHead.Total = 0;
    MmModifiedPageListHead.Quota = 0;
    MmModifiedPageListHead.Type = MmTypeModified;

    MmZeroedPageListHead.Total = 0;
    MmZeroedPageListHead.Quota = 16;
    MmZeroedPageListHead.Type = MmTypeZeroed;

    for ( Entry = 0, CurrentPfn = 0; Entry < PhysicalMap->EntryCount; Entry++ ) {


        if ( PhysicalMap->Entries[ Entry ].RegionType == 1 &&
             PhysicalMap->Entries[ Entry ].RegionLength >= 0x1000 ) {

            Length = PhysicalMap->Entries[ Entry ].RegionLength;
            Base = PhysicalMap->Entries[ Entry ].RegionBase;

            while ( Length > 0 ) {

                MmInsertPfnListEntry( &MmModifiedPageListHead, &MmPfnDatabase[ CurrentPfn ] );
                MmPfnDatabase[ CurrentPfn ].Long = 0;
                MmPfnDatabase[ CurrentPfn ].PageFrameNumber = Base >> 12;
                MmPfnDatabase[ CurrentPfn ].VaType = MmTypeModified;
                CurrentPfn++;

                Length -= 0x1000;
                Base += 0x1000;
            }
        }
    }

    MmNonPagedPoolLock = 0;

    //
    // We need to reserve the physicals that are occupied by 
    // newly added structures, all of this can be initialized to 
    // MmTypeSystemLocked.
    //
    // 0x000000 - 0x200000  This is mostly MMIO anyways, but it 
    //                      also contains LdrBootInfo and the original
    //                      system page tables.
    // 0x200000 - 0x400000  This is where the kernel is mapped and potentially
    //                      anything else required. (could be expanded to 0x600000)
    // 0x?????? - 0x??????  The mapping of OriginalPhysical and OriginalLength for the 
    //                      pfn database itself (and its page tables), this overlaps.
    //

    MmChangeDatabaseMemoryVaType( 0, 0x400000, MmTypeSystemLocked );
    MmChangeDatabaseMemoryVaType( OriginalPhysical, OriginalLength, MmTypeSystemLocked );

    ( ( PPMLE )__readcr3( ) )[ PAGE_MAP_INDEX_WORKING_SET ].PageFrameNumber = MmAllocatePhysical( MmTypePageTable ) >> 12;
    ( ( PPMLE )__readcr3( ) )[ PAGE_MAP_INDEX_WORKING_SET ].Present = 1;
    ( ( PPMLE )__readcr3( ) )[ PAGE_MAP_INDEX_WORKING_SET ].Write = 1;
    RtlZeroMemory( MiReferenceLevel4Entry( PAGE_MAP_INDEX_WORKING_SET ), 0x1000 );

    MiReferenceLevel4Entry( PAGE_MAP_INDEX_WORKING_SET )[ 0 ].PageFrameNumber = MmAllocatePhysical( MmTypePageTable ) >> 12;
    MiReferenceLevel4Entry( PAGE_MAP_INDEX_WORKING_SET )[ 0 ].Present = 1;
    MiReferenceLevel4Entry( PAGE_MAP_INDEX_WORKING_SET )[ 0 ].Write = 1;
    RtlZeroMemory( MiReferenceLevel3Entry( PAGE_MAP_INDEX_WORKING_SET, 0 ), 0x1000 );

    MiReferenceLevel3Entry( PAGE_MAP_INDEX_WORKING_SET, 0 )[ 0 ].PageFrameNumber = MmAllocatePhysical( MmTypePageTable ) >> 12;
    MiReferenceLevel3Entry( PAGE_MAP_INDEX_WORKING_SET, 0 )[ 0 ].Present = 1;
    MiReferenceLevel3Entry( PAGE_MAP_INDEX_WORKING_SET, 0 )[ 0 ].Write = 1;
    RtlZeroMemory( MiReferenceLevel2Entry( PAGE_MAP_INDEX_WORKING_SET, 0, 0 ), 0x1000 );

    MiReferenceLevel2Entry( PAGE_MAP_INDEX_WORKING_SET, 0, 0 )[ 0 ].PageFrameNumber = __readcr3( ) >> 12;
    MiReferenceLevel2Entry( PAGE_MAP_INDEX_WORKING_SET, 0, 0 )[ 0 ].Present = 1;
    MiReferenceLevel2Entry( PAGE_MAP_INDEX_WORKING_SET, 0, 0 )[ 0 ].Write = 1;

    MmAddressPageTable( 0 )[ MiIndexLevel1( 0 ) ].Long = 0;
    //MmAddressPageTable( 511 )[ MiIndexLevel1( 511 ) ].Long = 0;
    MiReferenceLevel2Entry( 511, 511, 511 )[ 511 ].Long = 0; // highest address (prevents stuff like null - 1 writes)
    MmMatchQuotaZeroListEvent( );
}

VOID
MmZeroMemoryThread(

)
{
    while ( TRUE ) {
        KeWaitForSingleObject( MiZeroQuotaEvent, WAIT_TIMEOUT_INFINITE );
        //RtlDebugPrint( L"ZERO. %d %d\n", MmZeroedPageListHead.Total, KeQueryCurrentProcessor( )->InService, MiZeroQuotaEvent );
        MmMatchQuotaZeroListEvent( );
        KeSignalEvent( MiZeroQuotaEvent, FALSE );
    }

}

VOID
MmPhase1InitializeMemoryManager(

)
{
    NTSTATUS ntStatus;
    OBJECT_ATTRIBUTES ObjectAttributes = {
        RTL_CONSTANT_STRING( L"\\Device\\SystemZeroQuota" ), { 0 }, OBJ_PERMANENT_OBJECT };
    HANDLE ThreadHandle;
    OBJECT_ATTRIBUTES ThreadAttributes = { { 0 }, { 0 }, OBJ_KERNEL_HANDLE };

    ntStatus = ObCreateObject( &MiZeroQuotaEvent,
                               KeEventObject,
                               &ObjectAttributes,
                               sizeof( KEVENT ) );
    if ( !NT_SUCCESS( ntStatus ) ) {

        KeBugCheck( STATUS_KERNEL_INITIALIZATION_FAILURE );
    }

    KeInitializeEvent( MiZeroQuotaEvent, TRUE );

    ULONG64 ThreadId;
    ntStatus = ZwCreateThread( &ThreadHandle,
                               ZwCurrentProcess( ),
                               THREAD_ALL_ACCESS,
                               ( PKSTART_ROUTINE )MmZeroMemoryThread,
                               NULL,
                               THREAD_SYSTEM,
                               &ThreadAttributes,
                               0,
                               &ThreadId );
    if ( !NT_SUCCESS( ntStatus ) ) {

        KeBugCheck( STATUS_KERNEL_INITIALIZATION_FAILURE );
    }
}
