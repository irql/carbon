


#pragma once

typedef struct _MM_VAD *PMM_VAD;

#define MM_TAG '  mM'

typedef union _CR3 {
    struct {
        ULONG64 Reserved1 : 3;
        ULONG64 WriteThrough : 1;
        ULONG64 CacheDisable : 1;
        ULONG64 PageTable : 36;
        ULONG64 Reserved3 : 16;
    };

    ULONG64     Value;
} CR3, *PCR3;

typedef union _PMLE {
    struct {
        ULONG64 Present : 1;
        ULONG64 Write : 1;
        ULONG64 User : 1;
        ULONG64 WriteThrough : 1;
        ULONG64 CacheDisable : 1;
        ULONG64 Accessed : 1;
        ULONG64 Dirty : 1;          // E
        ULONG64 Pat : 1;            // PAT if E, Large if T  
        ULONG64 Global : 1;         // E
        ULONG64 Avail1 : 3;
        ULONG64 PageFrameNumber : 36;
        ULONG64 Reserved1 : 4;
        ULONG64 Avail2 : 7;
        ULONG64 ProtectionKey : 4;  // E
        ULONG64 ExecuteDisable : 1; // E
    };

    ULONG64     Long;
} PMLE, *PPMLE;

C_ASSERT( sizeof( PMLE ) == 8 );

typedef enum _VA_TYPE {

    //
    // MmTypeModified is memory which is not completely
    // zero, but is also not in use, it is the default, 
    // and it can be used in allocations but the kernel
    // should use MmTypeZeroed if available.
    //

    MmTypeModified,

    //
    // MmTypeZeroed is the same as modified but it is 
    // all zeroed.
    //

    MmTypeZeroed,

    //
    // MmTypeSystemLocked is for memory which is locked by
    // the system, whether at boot-time or a later time, it
    // most-likely will never be freed.
    //

    MmTypeSystemLocked,

    //
    // MmTypePageTable is any memory allocated for a page table
    // entry or page table itself.
    //

    MmTypePageTable,

    //
    // MmTypeProcessPrivate describes memory allocated inside
    // any process context.
    //

    MmTypeProcessPrivate,

    //
    // MmTypeNonPagedPool is memory for any form of memory allocation 
    // inside the kernel, they're all grouped together inside a few 
    // page tables.
    //

    MmTypeNonPagedPool,

    MmTypeWorkingSetList,

    MmTypeSectionObject,

    MmTypeMaximum

} VA_TYPE;

typedef UCHAR MM_VA_TYPE;

typedef struct _MM_PFN {
    union {

        struct {
            //
            // This value has it's lower 12 bits cut off.
            // PageFrameNumber << 12 is the real physical.
            //

            ULONG64 PageFrameNumber : 32;

            //
            // This specifies the MMVATYPE for the physical
            // page which just tell's us what it is being used
            // for and if it is being used.
            //

            ULONG64 VaType : 8;

            //
            // This bit indicates if the current MMPFN
            // is in-use by the system, you should wait
            // until this is zero before touching.
            //

            ULONG64 LockBit : 1;

            //
            // Currently unused.
            //

            ULONG64 ReferenceCount : 16;
        };

        ULONG64     Long;
    };

    //
    // This field is used to link together pages by their type
    // it is only used for physical memory which is of one of the 
    // below types.
    // MmTypeModified, MmTypeZeroed
    //
    // You could optimize this field by using some smaller offset system,
    // or just saving MmPfnDatabase indexes.
    //

    LIST_ENTRY      PfnLinks;
} MM_PFN, *PMM_PFN;

C_ASSERT( sizeof( MM_PFN ) == 24 );

EXTERN PMM_PFN MmPfnDatabase;

typedef enum _MM_WSLE_USE {
    MmMappedUnused,
    MmMappedPhysical,
    MmMappedViewOfSection,
    MmWslUseMaximum
} MM_WSLE_USE;

typedef struct _MM_WSLE {
    union {
        struct {
            ULONG64 Usage : 8;
            ULONG64 Ignore : 56;
        };

        struct {
            ULONG64 Usage : 8;
            ULONG64 IndexPfn : 56;
            ULONG64 Address;
        } TypeMappedPhysical;

        struct {
            ULONG64 Usage : 8;
            ULONG64 SectionObject : 48; // x | 0xFFFF000000000000 (we could save 6 more bits.)
            ULONG64 LengthLower : 8;
            ULONG64 LengthUpper : 28; // LengthLower | (LengthUpper << 8) << 12
            ULONG64 Address : 36; // x << 12 (upper 16 are 0)
        } TypeMappedViewOfSection;

        struct {
            ULONG64 Upper;
            ULONG64 Lower;
        };
    };
} MM_WSLE, *PMM_WSLE;

C_ASSERT( sizeof( MM_WSLE ) == 16 );

typedef struct _MM_WSL {
    //
    // This structure will be used to manage physical
    // memory used by processes.
    //

    //
    // Each working set list entry is a PFN number, to the memory
    // which is currently in-use, this structure is also page aligned.
    //

    ULONG64 WorkingSetListCount;
    MM_WSLE WorkingSetList[ 255 ];

} MM_WSL, *PMM_WSL;

C_ASSERT( sizeof( MM_WSL ) <= 4096 );

EXTERN PMM_WSL MmCurrentWorkingSetList;

//EXTERN ULONG64 MmPageMapReferenceTable;
#define MmPageMapReferenceTable                 0xFFFFFF0000000000

#define PAGE_MAP_INDEX_KERNEL                   511ULL
#define PAGE_MAP_INDEX_REFERENCE                510ULL
#define PAGE_MAP_INDEX_PFN_DATABASE             509ULL
#define PAGE_MAP_INDEX_WORKING_SET              508ULL
#define PAGE_MAP_INDEX_NON_PAGED_POOL_END       507ULL
#define PAGE_MAP_INDEX_NON_PAGED_POOL_START     501ULL

#define MiIndexLevel4( address )                ( ( ( ULONG64 ) ( address ) & ( 0x1FFULL << 39ULL ) ) >> 39ULL )
#define MiIndexLevel3( address )                ( ( ( ULONG64 ) ( address ) & ( 0x1FFULL << 30ULL ) ) >> 30ULL )
#define MiIndexLevel2( address )                ( ( ( ULONG64 ) ( address ) & ( 0x1FFULL << 21ULL ) ) >> 21ULL )
#define MiIndexLevel1( address )                ( ( ( ULONG64 ) ( address ) & ( 0x1FFULL << 12ULL ) ) >> 12ULL )

#define MiConstructAddress( index4, index3, index2, index1 ) \
( ( PVOID )( ( ( ULONG64 )( index4 ) << 39ULL ) |\
( ( ULONG64 )( index3 ) << 30ULL ) |\
( ( ULONG64 )( index2 ) << 21ULL ) |\
( ( ULONG64 )( index1 ) << 12ULL ) |\
( ( ( ULONG64 )( index4 ) / 256 ) * 0xFFFF000000000000 ) ) )

#define MiLevel4Table                           ( ( PPMLE )0xFFFFFE0000000000 )

#define MiReferenceLevel4Entry( index4 ) \
( ( PPMLE )( ( ULONG64 )MmPageMapReferenceTable | \
( PAGE_MAP_INDEX_REFERENCE << 30ULL ) | \
( PAGE_MAP_INDEX_REFERENCE << 21ULL ) | \
( ( ( ULONG64 ) ( index4 ) & 0x1FFULL ) << 12ULL) ) )

#define MiReferenceLevel3Entry( index4, index3 ) \
( ( PPMLE )( ( ULONG64 )MmPageMapReferenceTable | \
( PAGE_MAP_INDEX_REFERENCE << 30ULL ) | \
( ( ( ULONG64 )( index4 ) & 0x1FFULL ) << 21ULL) | \
( ( ( ULONG64 )( index3 ) & 0x1FFULL ) << 12ULL) ) )

#define MiReferenceLevel2Entry( index4, index3, index2 ) \
( ( PPMLE )( ( ULONG64 )MmPageMapReferenceTable | \
( ( ( ULONG64 )( index4 ) & 0x1FFULL ) << 30ULL ) | \
( ( ( ULONG64 )( index3 ) & 0x1FFULL ) << 21ULL ) | \
( ( ( ULONG64 )( index2 ) & 0x1FFULL ) << 12ULL ) ) )

#define MiGetAddressSpace( )                    __readcr3( )
#define MiSetAddressSpace( address_space )      __writecr3( address_space )

EXTERN ULONG64 MmPfnCount;

typedef struct _MM_PFN_LIST { // add a spin lock.
    MM_VA_TYPE  Type;
    ULONG64     Total;
    ULONG32     Quota;
    PLIST_ENTRY Head;

} MM_PFN_LIST, *PMM_PFN_LIST;

EXTERN MM_PFN_LIST MmZeroedPageListHead;
EXTERN MM_PFN_LIST MmModifiedPageListHead;

EXTERN KSPIN_LOCK MmNonPagedPoolLock;

typedef struct _MM_POOL_HEAD {
    ULONG32     GranularTableCount;
    ULONG32     AllocatedTableCount;
    PLIST_ENTRY GranularTableLinks;
    PLIST_ENTRY AllocatedTableLinks;
} MM_POOL_HEAD, *PMM_POOL_HEAD;

EXTERN MM_POOL_HEAD MmNonPagedPoolHead;

typedef struct _MM_GRANULAR_PAGE_ENTRY {
    ULONG64   Page;
    ULONG64   Bitmap;
    POOL_TYPE Type;
} MM_GRANULAR_PAGE_ENTRY, *PMM_GRANULAR_PAGE_ENTRY;

C_ASSERT( sizeof( MM_GRANULAR_PAGE_ENTRY ) == 24 );

typedef struct _MM_ALLOCATED_PAGE_ENTRY {
    ULONG32     Tag;
    POOL_TYPE   Type;
    ULONG64     Address;
    ULONG64     Length;
} MM_ALLOCATED_PAGE_ENTRY, *PMM_ALLOCATED_PAGE_ENTRY;

C_ASSERT( sizeof( MM_ALLOCATED_PAGE_ENTRY ) == 24 );

typedef struct _MM_POOL_TABLE_HEADER {

    LIST_ENTRY              PoolLinks;

} MM_POOL_TABLE_HEADER, *PMM_POOL_TABLE_HEADER;

typedef struct _MM_POOL_TABLE_GRANULAR_PAGES {
    MM_POOL_TABLE_HEADER    Header;
    ULONG32                 Usage;
    MM_GRANULAR_PAGE_ENTRY  Entries[ 169 ];
} MM_POOL_TABLE_GRANULAR_PAGES, *PMM_POOL_TABLE_GRANULAR_PAGES;

typedef struct _MM_POOL_TABLE_ALLOCATED_PAGES {
    MM_POOL_TABLE_HEADER    Header;
    ULONG32                 Usage;
    MM_ALLOCATED_PAGE_ENTRY Entries[ 169 ];
} MM_POOL_TABLE_ALLOCATED_PAGES, *PMM_POOL_TABLE_ALLOCATED_PAGES;

C_ASSERT( sizeof( MM_POOL_TABLE_ALLOCATED_PAGES ) <= 4096 );
C_ASSERT( sizeof( MM_POOL_TABLE_GRANULAR_PAGES ) <= 4096 );

FORCEINLINE
VOID
MmInitializePfnListHead(
    _In_ PMM_PFN_LIST Head,
    _In_ PMM_PFN Pfn
)
{
    Head->Head = &Pfn->PfnLinks;
    Pfn->PfnLinks.Flink = &Pfn->PfnLinks;
    Pfn->PfnLinks.Blink = &Pfn->PfnLinks;
    Head->Total++;
}

FORCEINLINE
VOID
MmRemovePfnListEntry(
    _In_ PMM_PFN_LIST Head,
    _In_ PMM_PFN Pfn
)
{
    PLIST_ENTRY Flink;
    PLIST_ENTRY Blink;

    Flink = Pfn->PfnLinks.Flink;
    Blink = Pfn->PfnLinks.Blink;
    Blink->Flink = Flink;
    Flink->Blink = Blink;

    if ( Head->Head == &Pfn->PfnLinks ) {
        Head->Head = Flink;
    }
    Head->Total--;
}

FORCEINLINE
VOID
MmInsertPfnListEntry(
    _In_ PMM_PFN_LIST Head,
    _In_ PMM_PFN Pfn
)
{
    //PLIST_ENTRY Flink;

    if ( Head->Total == 0 ) {
        MmInitializePfnListHead( Head, Pfn );
        return;
    }

#if 0
    Flink = Head->Head->Flink;
    Pfn->PfnLinks.Flink = Flink;
    Pfn->PfnLinks.Blink = Head->Head;
    Flink->Blink = &Pfn->PfnLinks;
    Head->Head->Flink = &Pfn->PfnLinks;
#endif
    KeInsertEntryTail( Head->Head, &Pfn->PfnLinks );
    Head->Total++;
}

VOID
MmInitializeMemoryManager(
    _In_ PMEMORY_MAP PhysicalMap
);

VOID
MmChangeDatabaseMemoryVaType(
    _In_ ULONG64  Address,
    _In_ ULONG64  Length,
    _In_ MM_VA_TYPE Type
);

PMM_PFN
MmReferenceDatabaseByAddress(
    _In_ ULONG64 Address
);

PMM_PFN
MmReferenceDatabaseByType(
    _In_ MM_VA_TYPE Type
);

PMM_PFN
MmReferenceDatabaseByLinkedType(
    _In_ MM_VA_TYPE Type
);

VOID
MmMatchQuotaZeroListEvent(

);

VOID
MmChangePfnVaType(
    _In_ PMM_PFN   Pfn,
    _In_ MM_VA_TYPE Type
);

PPMLE
MmAddressPageTableByIndex(
    _In_ ULONG64 Level4,
    _In_ ULONG64 Level3,
    _In_ ULONG64 Level2
);

PPMLE
MmAddressPageTable(
    _In_ ULONG64 Address
);

ULONG64
MmAllocatePhysical(
    _In_ MM_VA_TYPE Reason
);

VOID
MmFreePhysical(
    _In_ ULONG64 Address
);

VOID
MmFlushAddress(
    _In_ PVOID Address
);

VOID
MmPhase1InitializeMemoryManager(

);

ULONG64
MmAllocateZeroedPhysical(
    _In_  MM_VA_TYPE Reason,
    _Out_ PBOOLEAN   Zeroed
);

EXTERN PKEVENT MiZeroQuotaEvent;

typedef struct _MM_SECTION_CLUSTER *PMM_SECTION_CLUSTER;
typedef struct _MM_SECTION_CLUSTER {
    ULONG64             Address[ 15 ];
    PMM_SECTION_CLUSTER Link;
} MM_SECTION_CLUSTER, *PMM_SECTION_CLUSTER;

C_ASSERT( sizeof( MM_SECTION_CLUSTER ) == 128 );

//
// Internal flag to indicate to MmMapViewOfSection that
// it needs to map the file object.
//

#define SEC_UNINITIALIZED_FO (0x80000000)

typedef struct _MM_SECTION_OBJECT {
    ULONG32             AllocationAttributes;
    union {
        ULONG64         Address;
        PIO_FILE_OBJECT FileObject;
    };
    ULONG64             Length;
    ULONG64             LockedBase;
    PMM_SECTION_CLUSTER FirstCluster;

} MM_SECTION_OBJECT, *PMM_SECTION_OBJECT;

PVOID
MiFindFreeNonPagedPoolRegion(
    _In_ ULONG64 PageLength
);

VOID
MmInsertWorkingSet(
    _In_ PMM_WSLE  Entry
);

PMM_WSLE
MmFindWorkingSetByAddress(
    _In_ MM_WSLE_USE Usage,
    _In_ ULONG64     Address
);

VOID
MmFreeWorkingSetListEntry(
    _In_ PMM_WSLE Entry
);

#define IA32_MTRR_CAPABILITIES 0x000000FE
#define IA32_MTRR_PHYSBASE0    0x00000200
#define IA32_MTRR_PHYSMASK0    0x00000201
#define IA32_MTRR_DEF_TYPE     0x000002FF

typedef union _IA32_PAT_MSR {
    struct {
        ULONG64 Pa0 : 3;
        ULONG64 Reserved0 : 5;
        ULONG64 Pa1 : 3;
        ULONG64 Reserved1 : 5;
        ULONG64 Pa2 : 3;
        ULONG64 Reserved2 : 5;
        ULONG64 Pa3 : 3;
        ULONG64 Reserved3 : 5;
        ULONG64 Pa4 : 3;
        ULONG64 Reserved4 : 5;
        ULONG64 Pa5 : 3;
        ULONG64 Reserved5 : 5;
        ULONG64 Pa6 : 3;
        ULONG64 Reserved6 : 5;
        ULONG64 Pa7 : 3;
        ULONG64 Reserved7 : 5;
    };

    ULONG64     Long;
} IA32_PAT_MSR, *PIA32_PAT_MSR;

C_ASSERT( sizeof( IA32_PAT_MSR ) == 8 );

#define IA32_MSR_PAT    0x00000277

#define MEM_TYPE_UC     0x00
#define MEM_TYPE_WC     0x01
#define MEM_TYPE_WT     0x04
#define MEM_TYPE_WP     0x05
#define MEM_TYPE_WB     0x06
#define MEM_TYPE_UC1    0x07

VOID
MmInitializeCaching(

);

BOOLEAN
MiIsRegionFree(
    _In_ PVOID     PageAddress,
    _In_ ULONG64   PageLength
);

PVOID
MiFindFreeUserRegion(
    _In_ PKPROCESS Process,
    _In_ ULONG64   PageLength
);

PMM_SECTION_CLUSTER
MiCreateSectionCluster(

);

VOID
MiInsertSectionAddress(
    _In_ PMM_SECTION_OBJECT Section,
    _In_ ULONG64            Address,
    _In_ ULONG64            PageLength
);

VOID
MiCleanupSection(
    _In_ PMM_SECTION_OBJECT Section
);

ULONG64
MmAllocateZeroedPhysicalWithPfn(
    _In_      MM_VA_TYPE Reason,
    _Out_     PBOOLEAN   Zeroed,
    _Out_opt_ PMM_PFN*   Pfn
);

ULONG64
MmAllocatePhysicalWithPfn(
    _In_      MM_VA_TYPE Reason,
    _Out_opt_ PMM_PFN*   Pfn
);

PMM_VAD
MiAllocateVad(

);

VOID
MiFreeVad(
    _In_ PMM_VAD Vad
);

VOID
MiInsertVad(
    _In_ PKPROCESS       Process,
    _In_ PMM_VAD         Vad
);

PMM_VAD
MiFindVadByFullName(
    _In_ PKPROCESS       Process,
    _In_ PUNICODE_STRING FileName
);

PMM_VAD
MiFindVadByShortName(
    _In_ PKPROCESS       Process,
    _In_ PUNICODE_STRING ShortName
);

PMM_VAD
MiFindVadByAddress(
    _In_ PKPROCESS Process,
    _In_ ULONG64   Base
);

VOID
MiRemoveVadByPointer(
    _In_ PKPROCESS Process,
    _In_ PMM_VAD   Vad
);

ULONG64
MmCreateAddressSpace(

);
