


#include <carbusr.h>
#include "../ntdll.h"

//
// This is going to be a simple heap implementation just
// for malloc/free shit like that
//
// ngl i have no idea about heap implementations and i dont care
//

typedef struct _RTL_RESERVED_REGION {
    LIST_ENTRY ReservedLinks;
    ULONG64    Bitmap;

} RTL_RESERVED_REGION, *PRTL_RESERVED_REGION;

C_ASSERT( sizeof( RTL_RESERVED_REGION ) <= 0x40 );

typedef struct _RTL_COMMITTED_REGION {
    LIST_ENTRY CommittedLinks;

    struct {
        // bit 1 indicates if an entry is in use.
        // bit 2 is set if it's a pointer to a RTL_RESERVED_REGION
        ULONG64 Base;
        ULONG64 Length;
    } Entries[ 255 ];

} RTL_COMMITTED_REGION, *PRTL_COMMITTED_REGION;

C_ASSERT( sizeof( RTL_COMMITTED_REGION ) <= 0x1000 );

typedef struct _RTL_HEAP_HANDLE {
    PLIST_ENTRY Reserved;
    PLIST_ENTRY Committed;
    ULONG64     ReservedCount;
    ULONG64     CommittedCount;
    HANDLE      HeapLock;

    //
    // Probably add charge values.
    //

} RTL_HEAP_HANDLE, *PRTL_HEAP_HANDLE;

//
// Need to add reserve/commit memory 
// Need to add peb
//

PVOID
RtlCreateHeap(

)
{
    PRTL_HEAP_HANDLE HeapHandle;

    HeapHandle = NULL;
    NtAllocateVirtualMemory( NtCurrentProcess( ),
                             &HeapHandle,
                             sizeof( RTL_HEAP_HANDLE ),
                             PAGE_READ | PAGE_WRITE );


    NtCreateMutex( &HeapHandle->HeapLock, FALSE );

    return HeapHandle;
}

VOID
RtlpCommitHeap(
    _In_ PRTL_HEAP_HANDLE HeapHandle,
    _In_ ULONG64          Base,
    _In_ ULONG64          Length
)
{
    NTSTATUS ntStatus;
    PRTL_COMMITTED_REGION Commit;
    PLIST_ENTRY Flink;
    ULONG64 Current;

    if ( HeapHandle->CommittedCount > 0 ) {

        Flink = HeapHandle->Committed;

        do {
            Commit = CONTAINING_RECORD( Flink, RTL_COMMITTED_REGION, CommittedLinks );

            for ( Current = 0; Current < 255; Current++ ) {

                if ( ( Commit->Entries[ Current ].Base & 1 ) == 0 ) {

                    Commit->Entries[ Current ].Base = Base | 1;
                    Commit->Entries[ Current ].Length = Length;
                    return;
                }
            }

            Flink = Flink->Flink;
        } while ( Flink != HeapHandle->Committed );
    }

    Commit = NULL;
    ntStatus = NtAllocateVirtualMemory( NtCurrentProcess( ),
                                        &Commit,
                                        sizeof( RTL_COMMITTED_REGION ),
                                        PAGE_READ | PAGE_WRITE );
    Commit->Entries[ 0 ].Base = Base | 1;
    Commit->Entries[ 0 ].Length = Length;

    if ( HeapHandle->CommittedCount == 0 ) {

        KeInitializeListHead( &Commit->CommittedLinks );
        HeapHandle->Committed = &Commit->CommittedLinks;
    }
    else {

        KeInsertEntryTail( HeapHandle->Committed, &Commit->CommittedLinks );
    }
    HeapHandle->CommittedCount++;

}

PVOID
RtlAllocateHeap(
    _In_ PVOID   Heap,
    _In_ ULONG64 Length
)
{
    NTSTATUS ntStatus;
    PRTL_HEAP_HANDLE HeapHandle;
    PRTL_RESERVED_REGION Reserve;
    PLIST_ENTRY Flink;
    ULONG64 Bit;
    ULONG64 SubLength;
    ULONG64 BitMask;
    PVOID Base;

    HeapHandle = Heap;

    NtWaitForSingleObject( HeapHandle->HeapLock, WAIT_TIMEOUT_INFINITE );

    Length = ROUND( Length, 0x40 );

    if ( Length < 0x1000 ) {
        Flink = HeapHandle->Reserved;
        SubLength = Length >> 6;
        BitMask = 0;

        for ( int i = 0; i < SubLength; i++ ) {

            BitMask |= 1ULL << i;
        }

        if ( HeapHandle->ReservedCount > 0 ) {
            do {
                Reserve = CONTAINING_RECORD( Flink, RTL_RESERVED_REGION, ReservedLinks );

                for ( Bit = 0; Bit < 64 - SubLength; Bit++ ) {

                    if ( ( Reserve->Bitmap & ( BitMask << Bit ) ) == 0 ) {

                        Reserve->Bitmap |= BitMask << Bit;

                        RtlpCommitHeap( HeapHandle, ( ULONG64 )Reserve + ( Bit << 6 ) | 2, Length );
                        NtReleaseMutex( HeapHandle->HeapLock );
                        return ( PVOID )( ( ULONG64 )Reserve + ( Bit << 6 ) );
                    }
                }

                Flink = Flink->Flink;
            } while ( Flink != HeapHandle->Reserved );
        }

        //
        // If no reserved region had enough free space or there are no reserved
        // regions, we create a new one here.
        //

        Reserve = NULL;
        ntStatus = NtAllocateVirtualMemory( NtCurrentProcess( ),
                                            &Reserve,
                                            0x1000,
                                            PAGE_READ | PAGE_WRITE );

        if ( !NT_SUCCESS( ntStatus ) ) {

            NtReleaseMutex( HeapHandle->HeapLock );
            return NULL;
        }

        Reserve->Bitmap = 1 | ( BitMask << 1 );

        if ( HeapHandle->ReservedCount == 0 ) {

            KeInitializeListHead( &Reserve->ReservedLinks );
            HeapHandle->Reserved = &Reserve->ReservedLinks;
        }
        else {

            KeInsertEntryTail( HeapHandle->Reserved, &Reserve->ReservedLinks );
        }
        HeapHandle->ReservedCount++;

        RtlpCommitHeap( HeapHandle, ( ULONG64 )Reserve + ( 1 << 6 ) | 2, Length );
        NtReleaseMutex( HeapHandle->HeapLock );
        return ( PVOID )( ( ULONG64 )Reserve + ( 1 << 6 ) );
    }
    else {
        Base = NULL;
        NtAllocateVirtualMemory( NtCurrentProcess( ),
                                 &Base,
                                 Length,
                                 PAGE_READ | PAGE_WRITE );
        RtlpCommitHeap( HeapHandle, ( ULONG64 )Base, Length );
        NtReleaseMutex( HeapHandle->HeapLock );
        return Base;
    }
}

// add mutex wait to these two, cba opening this file atm
VOID
RtlFreeHeap(
    _In_ PVOID Heap,
    _In_ PVOID Memory
)
{
    PRTL_HEAP_HANDLE HeapHandle;
    PRTL_COMMITTED_REGION Commit;
    PRTL_RESERVED_REGION Reserved;
    PLIST_ENTRY Flink;
    ULONG64 Current;
    ULONG64 BitMask;
    ULONG64 Bit;

    HeapHandle = Heap;

    NtWaitForSingleObject( HeapHandle->HeapLock, WAIT_TIMEOUT_INFINITE );

    if ( HeapHandle->CommittedCount > 0 ) {

        Flink = HeapHandle->Committed;
        do {
            Commit = CONTAINING_RECORD( Flink, RTL_COMMITTED_REGION, CommittedLinks );

            for ( Current = 0; Current < 255; Current++ ) {

                if ( ( ULONG64 )Memory == ( Commit->Entries[ Current ].Base & ~3 ) ) {

                    if ( Commit->Entries[ Current ].Base & 2 ) {
                        Reserved = ( PRTL_RESERVED_REGION )( Commit->Entries[ Current ].Base & ~0xFFF );
                        Bit = ( Commit->Entries[ Current ].Base & 0xFFC ) >> 6;
                        BitMask = 0;

                        for ( int i = 0; i < Commit->Entries[ Current ].Length >> 6; i++ ) {

                            BitMask |= 1ULL << i;
                        }

                        Reserved->Bitmap &= ~( BitMask << Bit );

                        if ( Reserved->Bitmap == 1 ) {

                            KeRemoveListEntry( &Reserved->ReservedLinks );
                            NtFreeVirtualMemory( NtCurrentProcess( ),
                                                 Reserved,
                                                 0x1000 );
                        }
                    }
                    else {

                        NtFreeVirtualMemory( NtCurrentProcess( ),
                            ( PVOID )( Commit->Entries[ Current ].Base & ~1 ),
                                             Commit->Entries[ Current ].Length );
                    }

                    Commit->Entries[ Current ].Base &= ~1;
                    NtReleaseMutex( HeapHandle->HeapLock );
                    return;
                }
            }

            Flink = Flink->Flink;
        } while ( Flink != HeapHandle->Committed );
    }

    //jon is bussin
}

ULONG64
RtlAllocationSizeHeap(
    _In_ PVOID Heap,
    _In_ PVOID Memory
)
{
    PRTL_HEAP_HANDLE HeapHandle;
    PRTL_COMMITTED_REGION Commit;
    PLIST_ENTRY Flink;
    ULONG64 Current;

    HeapHandle = Heap;

    NtWaitForSingleObject( HeapHandle->HeapLock, WAIT_TIMEOUT_INFINITE );

    if ( HeapHandle->CommittedCount > 0 ) {

        Flink = HeapHandle->Committed;
        do {
            Commit = CONTAINING_RECORD( Flink, RTL_COMMITTED_REGION, CommittedLinks );

            for ( Current = 0; Current < 255; Current++ ) {

                if ( ( ULONG64 )Memory == ( Commit->Entries[ Current ].Base & ~3 ) ) {

                    NtReleaseMutex( HeapHandle->HeapLock );
                    return Commit->Entries[ Current ].Length;
                }
            }

            Flink = Flink->Flink;
        } while ( Flink != HeapHandle->Committed );
    }

    NtReleaseMutex( HeapHandle->HeapLock );
    return 0;
}
