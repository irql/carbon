


#include <carbsup.h>
#include "mi.h"
#include "../hal/halp.h"
#include "../ke/ki.h"

ULONG64
MmCreateAddressSpace(

)
{
    ULONG64 Level4;
    ULONG64 Pcid;
    PVOID MappedLevel4;
    ULONG64 PreviousAddressSpace;

    Pcid = 0;

    if ( KeProcessorFeatureEnabled( KeQueryCurrentProcessor( ),
                                    KPF_PCID_ENABLED ) ) {

        Pcid = MiAllocatePcid( );
    }

    Level4 = MmAllocatePhysical( MmTypePageTable );
    MappedLevel4 = MmMapIoSpace( Level4, 0x1000 );
    RtlZeroMemory( MappedLevel4, 0x800 );

    RtlCopyMemory( ( PVOID )( ( ULONG64 )MappedLevel4 + 0x800 ), MiLevel4Table + 256, 0x800 );

    ( ( PPMLE )MappedLevel4 )[ PAGE_MAP_INDEX_REFERENCE ].Long = 0;
    ( ( PPMLE )MappedLevel4 )[ PAGE_MAP_INDEX_REFERENCE ].PageFrameNumber = Level4 >> 12;
    ( ( PPMLE )MappedLevel4 )[ PAGE_MAP_INDEX_REFERENCE ].Present = 1;
    ( ( PPMLE )MappedLevel4 )[ PAGE_MAP_INDEX_REFERENCE ].Write = 1;

    ( ( PPMLE )MappedLevel4 )[ PAGE_MAP_INDEX_WORKING_SET ].Long = 0;
    ( ( PPMLE )MappedLevel4 )[ PAGE_MAP_INDEX_WORKING_SET ].PageFrameNumber = MmAllocatePhysical( MmTypePageTable ) >> 12;
    ( ( PPMLE )MappedLevel4 )[ PAGE_MAP_INDEX_WORKING_SET ].Present = 1;
    ( ( PPMLE )MappedLevel4 )[ PAGE_MAP_INDEX_WORKING_SET ].Write = 1;

    PreviousAddressSpace = MiGetAddressSpace( );
    MiSetAddressSpace( Level4 | Pcid );

    RtlZeroMemory( MiReferenceLevel4Entry( PAGE_MAP_INDEX_WORKING_SET ), 0x1000 );

    MiReferenceLevel4Entry( PAGE_MAP_INDEX_WORKING_SET )[ 0 ].PageFrameNumber = MmAllocatePhysical( MmTypePageTable ) >> 12;
    MiReferenceLevel4Entry( PAGE_MAP_INDEX_WORKING_SET )[ 0 ].Present = 1;
    MiReferenceLevel4Entry( PAGE_MAP_INDEX_WORKING_SET )[ 0 ].Write = 1;
    RtlZeroMemory( MiReferenceLevel3Entry( PAGE_MAP_INDEX_WORKING_SET, 0 ), 0x1000 );

    MiReferenceLevel3Entry( PAGE_MAP_INDEX_WORKING_SET, 0 )[ 0 ].PageFrameNumber = MmAllocatePhysical( MmTypePageTable ) >> 12;
    MiReferenceLevel3Entry( PAGE_MAP_INDEX_WORKING_SET, 0 )[ 0 ].Present = 1;
    MiReferenceLevel3Entry( PAGE_MAP_INDEX_WORKING_SET, 0 )[ 0 ].Write = 1;
    RtlZeroMemory( MiReferenceLevel2Entry( PAGE_MAP_INDEX_WORKING_SET, 0, 0 ), 0x1000 );

    MiReferenceLevel2Entry( PAGE_MAP_INDEX_WORKING_SET, 0, 0 )[ 0 ].PageFrameNumber = Level4 >> 12;
    MiReferenceLevel2Entry( PAGE_MAP_INDEX_WORKING_SET, 0, 0 )[ 0 ].Present = 1;
    MiReferenceLevel2Entry( PAGE_MAP_INDEX_WORKING_SET, 0, 0 )[ 0 ].Write = 1;

    MiSetAddressSpace( PreviousAddressSpace );

    MmUnmapIoSpace( MappedLevel4 );
    return Level4 | Pcid;
}

NTSTATUS
ZwAllocateVirtualMemory(
    _In_    HANDLE  ProcessHandle,
    _Inout_ PVOID*  BaseAddress,
    _In_    ULONG64 Length,
    _In_    ULONG32 Protect
)
{
    NTSTATUS ntStatus;
    PKPROCESS Process;
    ULONG64 PageLength;
    ULONG64 CurrentPage;
    ULONG64 PageAddress;
    BOOLEAN Zeroed;
    PPMLE PageTable;
    MM_WSLE Physical;
    PMM_PFN Pfn;
    KIRQL PreviousIrql;
    ULONG64 PreviousAddressSpace;

    Length = ROUND_TO_PAGES( Length );
    PageLength = Length >> 12;
    Physical.Upper = 0;
    Physical.Lower = 0;
    Physical.Usage = MmMappedPhysical;

    ntStatus = ObReferenceObjectByHandle( &Process,
                                          ProcessHandle,
                                          PROCESS_VM_OPERATION,
                                          KernelMode,
                                          PsProcessObject );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    PreviousAddressSpace = MiGetAddressSpace( );
    MiSetAddressSpace( Process->DirectoryTableBase );

    KeAcquireSpinLock( &Process->UserRegionLock, &PreviousIrql );
    KeAcquireSpinLockAtDpcLevel( &Process->WorkingSetLock );

    if ( *BaseAddress == NULL ) {

        PageAddress = ( ULONG64 )MiFindFreeUserRegion( Process, PageLength );
        *BaseAddress = ( PVOID )PageAddress;
    }
    else {
        *BaseAddress = ( PVOID )ROUND_TO_PAGES( *BaseAddress );

        if ( !MiIsRegionFree( *BaseAddress, PageLength ) ) {

            KeReleaseSpinLockAtDpcLevel( &Process->WorkingSetLock );
            KeReleaseSpinLock( &Process->UserRegionLock, PreviousIrql );
            return STATUS_INVALID_ADDRESS;
        }

        PageAddress = ( ULONG64 )*BaseAddress;
    }

    for ( CurrentPage = 0; CurrentPage < PageLength; CurrentPage++ ) {

        PageTable = MmAddressPageTable( PageAddress + ( CurrentPage << 12 ) );

        PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].PageFrameNumber =
            MmAllocateZeroedPhysicalWithPfn( MmTypeProcessPrivate, &Zeroed, &Pfn ) >> 12;
        PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].Write = ( Protect & PAGE_WRITE ) == PAGE_WRITE;

        if ( KeProcessorFeatureEnabled( KeQueryCurrentProcessor( ), KPF_NX_ENABLED ) ) {

            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].ExecuteDisable = ( Protect & PAGE_EXECUTE ) != PAGE_EXECUTE;
        }

        if ( ( Protect & PAGE_NOCACHE ) == PAGE_NOCACHE ) {
            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].Pat = 0;
            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].CacheDisable = 1;
            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].WriteThrough = 0; // Pa2
        }
        else if ( ( Protect & PAGE_WRITECOMBINE ) == PAGE_WRITECOMBINE ) {
            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].Pat = 0;
            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].CacheDisable = 0;
            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].WriteThrough = 1; // Pa1
        }
        else {
            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].Pat = 0;
            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].CacheDisable = 0;
            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].WriteThrough = 0; // Pa0
        }

        PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].User = 1;
        PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].Present = 1;

        if ( !Zeroed ) {
            RtlZeroMemory( ( PVOID )( PageAddress + ( CurrentPage << 12 ) ), 0x1000 );
        }

        Physical.Logical.Address = PageAddress + ( CurrentPage << 12 );
        Physical.Logical.IndexPfn = ( ( PUCHAR )Pfn - ( PUCHAR )MmPfnDatabase ) / sizeof( MM_PFN );
        MmInsertWorkingSet( &Physical );
    }

    KeReleaseSpinLockAtDpcLevel( &Process->WorkingSetLock );
    KeReleaseSpinLock( &Process->UserRegionLock, PreviousIrql );

    MiSetAddressSpace( PreviousAddressSpace );

    ObDereferenceObject( Process );
    return STATUS_SUCCESS;
}

NTSTATUS
ZwFreeVirtualMemory(
    _In_ HANDLE  ProcessHandle,
    _In_ PVOID   BaseAddress,
    _In_ ULONG64 Length
)
{
    NTSTATUS ntStatus;
    PKPROCESS Process;
    ULONG64 PageLength;
    ULONG64 CurrentPage;
    ULONG64 PageAddress;
    PPMLE PageTable;
    PMM_WSLE Physical;
    KIRQL PreviousIrql;
    ULONG64 PreviousAddressSpace;

    Length = ROUND_TO_PAGES( Length );
    PageLength = Length >> 12;
    PageAddress = ( ULONG64 )BaseAddress;

    ntStatus = ObReferenceObjectByHandle( &Process,
                                          ProcessHandle,
                                          PROCESS_VM_OPERATION,
                                          KernelMode,
                                          PsProcessObject );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    PreviousAddressSpace = MiGetAddressSpace( );
    MiSetAddressSpace( Process->DirectoryTableBase );

    KeAcquireSpinLock( &Process->UserRegionLock, &PreviousIrql );
    KeAcquireSpinLockAtDpcLevel( &Process->WorkingSetLock );

    for ( CurrentPage = 0; CurrentPage < PageLength; CurrentPage++ ) {

        Physical = MmFindWorkingSetByAddress( MmMappedPhysical, PageAddress + ( CurrentPage << 12 ) );

        if ( Physical == NULL ) {

            KeReleaseSpinLockAtDpcLevel( &Process->WorkingSetLock );
            KeReleaseSpinLock( &Process->UserRegionLock, PreviousIrql );
            MiSetAddressSpace( PreviousAddressSpace );
            ObDereferenceObject( Process );
            return CurrentPage == 0 ? STATUS_INVALID_ADDRESS : STATUS_PARTIALLY_COMPLETE;
        }

        PageTable = MmAddressPageTable( Physical->Logical.Address );
        PageTable[ MiIndexLevel1( Physical->Logical.Address ) ].Long = 0;
        MmFlushAddress( ( PVOID )Physical->Logical.Address );

        MmPfnDatabase->LockBit = TRUE;
        MmPfnDatabase[ Physical->Logical.IndexPfn ].ReferenceCount--;
        if ( MmPfnDatabase[ Physical->Logical.IndexPfn ].ReferenceCount == 0 ) {
            MmChangePfnVaType( &MmPfnDatabase[ Physical->Logical.IndexPfn ], MmTypeModified );
        }
        MmPfnDatabase->LockBit = FALSE;

        MmFreeWorkingSetListEntry( Physical );
    }

    KeAcquireSpinLockAtDpcLevel( &Process->WorkingSetLock );
    KeReleaseSpinLock( &Process->UserRegionLock, PreviousIrql );
    MiSetAddressSpace( PreviousAddressSpace );
    ObDereferenceObject( Process );
    return STATUS_SUCCESS;
}

NTSTATUS
NtAllocateVirtualMemory(
    _In_    HANDLE  ProcessHandle,
    _Inout_ PVOID*  BaseAddress,
    _In_    ULONG64 Length,
    _In_    ULONG32 Protect
)
{
    __try {

        return ZwAllocateVirtualMemory( ProcessHandle,
                                        BaseAddress,
                                        Length,
                                        Protect );
    }
    __except ( EXCEPTION_EXECUTE_HANDLER ) {

        RtlRaiseException( STATUS_ACCESS_VIOLATION );
    }
}

NTSTATUS
NtFreeVirtualMemory(
    _In_ HANDLE  ProcessHandle,
    _In_ PVOID   BaseAddress,
    _In_ ULONG64 Length
)
{
    __try {

        return ZwFreeVirtualMemory( ProcessHandle,
                                    BaseAddress,
                                    Length );
    }
    __except ( EXCEPTION_EXECUTE_HANDLER ) {

        RtlRaiseException( STATUS_ACCESS_VIOLATION );
    }
}
