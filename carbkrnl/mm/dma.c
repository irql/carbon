


#include <carbsup.h>
#include "mi.h"

EXTERN POBJECT_TYPE MmDmaAdapterObject;

NTSTATUS
MmDmaCreateAdapter(
    _In_     PDEVICE_OBJECT   DeviceObject,
    _In_     ULONG64          LogicalMask,
    _In_     ULONG64          DmaMode,
    _In_     ULONG64          CacheMode,
    _In_opt_ ULONG64          Length,
    _Out_    PMM_DMA_ADAPTER* Adapter
)
{
    NTSTATUS ntStatus;
    OBJECT_ATTRIBUTES Dma = { { 0 }, { 0 }, OBJ_PERMANENT_OBJECT };
    PMM_DMA_ADAPTER AdapterObject;

    ntStatus = ObCreateObject( &AdapterObject,
                               MmDmaAdapterObject,
                               &Dma,
                               sizeof( MM_DMA_ADAPTER ) );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    AdapterObject->DmaMode = MmDmaDefault;
    AdapterObject->CacheMode = CacheMode;
    AdapterObject->DeviceObject = DeviceObject;
    AdapterObject->LogicalMask = LogicalMask >> 12;
    *Adapter = AdapterObject;

    if ( DmaMode == MmDmaPrepared ||
         DmaMode == MmDmaSemiPrepared ) {

        //
        // This mode has a buffer pre-allocated for it
        // so when MmDmaAllocateBuffer is called, it simply
        // returns the associated buffers.
        //
        // MmDmaAllocateBuffer just reads MmPfnDatabase because
        // initmm.c should have addresses be ordered by their 
        // physical range - meaning it is faster than reading
        // the links over & over - but still extremely slow.
        //

        ntStatus = MmDmaAllocateBuffer( AdapterObject,
                                        Length,
                                        &AdapterObject->LogicalAddress,
                                        &AdapterObject->VirtualAddress );

        if ( !NT_SUCCESS( ntStatus ) ) {

            //RtlDebugPrint( L"Failed to allocate dma buffer: %ul\n", ntStatus );
            ObDereferenceObject( AdapterObject );
            return ntStatus;
        }

        AdapterObject->DmaMode = DmaMode;
        /*
        RtlDebugPrint( L"MmDmaPrepared buffer: %ull %ull\n",
                       AdapterObject->LogicalAddress,
                       AdapterObject->VirtualAddress );*/
    }

    return STATUS_SUCCESS;
}

NTSTATUS
MmDmaAllocateBuffer(
    _In_  PMM_DMA_ADAPTER AdapterObject,
    _In_  ULONG64         Length,
    _Out_ ULONG64*        LogicalAddress,
    _Out_ ULONG64*        VirtualAddress
)
{
    ULONG64 PfnCurrent;
    ULONG64 PfnRequested;
    ULONG64 PfnFoundStart;
    ULONG64 PfnFoundTotal;

    if ( AdapterObject->DmaMode == MmDmaPrepared ||
        ( AdapterObject->DmaMode == MmDmaSemiPrepared &&
          Length == 0 ) ) {

        *LogicalAddress = AdapterObject->LogicalAddress;
        *VirtualAddress = AdapterObject->VirtualAddress;
        return STATUS_SUCCESS;
    }

    Length = ROUND_TO_PAGES( Length );

    PfnRequested = Length >> 12;
    PfnFoundStart = 0;
    PfnFoundTotal = 0;

    for ( PfnCurrent = 0; PfnCurrent < MmPfnCount; PfnCurrent++ ) {

        if ( ( MmPfnDatabase[ PfnCurrent ].VaType != MmTypeModified &&
               MmPfnDatabase[ PfnCurrent ].VaType != MmTypeZeroed ) ||
             MmPfnDatabase[ PfnCurrent ].LockBit == TRUE ) {

            while ( PfnFoundTotal-- ) {

                MmPfnDatabase[ PfnFoundStart + PfnFoundTotal ].LockBit = FALSE;
            }

            PfnFoundStart = 0;
            PfnFoundTotal = 0;
            continue;
        }

        if ( PfnFoundTotal == 0 &&
            ( MmPfnDatabase[ PfnCurrent ].PageFrameNumber & AdapterObject->LogicalMask ) !=
             MmPfnDatabase[ PfnCurrent ].PageFrameNumber ) {

            continue;
        }

        if ( PfnFoundTotal > 0 &&
            ( MmPfnDatabase[ PfnFoundStart + PfnFoundTotal - 1 ].PageFrameNumber !=
              MmPfnDatabase[ PfnCurrent ].PageFrameNumber - 1 ||
              MmPfnDatabase[ PfnCurrent ].PageFrameNumber > AdapterObject->LogicalMask ) ) {

            while ( PfnFoundTotal-- ) {

                MmPfnDatabase[ PfnFoundStart + PfnFoundTotal ].LockBit = FALSE;
            }

            PfnFoundStart = 0;
            PfnFoundTotal = 0;
            continue;
        }

        if ( PfnFoundTotal == 0 ) {

            PfnFoundStart = PfnCurrent;
        }
        PfnFoundTotal++;

        MmPfnDatabase[ PfnCurrent ].LockBit = TRUE;

        if ( PfnFoundTotal >= PfnRequested ) {

            // should never be allocated or touched anyways 
            // especially not by MmMapIoSpaceSpecifyCache
            // but i think it is being ? TODO: fix
            while ( PfnFoundTotal-- ) {

                MmChangePfnVaType( MmPfnDatabase + PfnFoundStart + PfnFoundTotal,
                                   MmTypeDma );
                MmPfnDatabase[ PfnFoundStart + PfnFoundTotal ].LockBit = FALSE;
                MmPfnDatabase[ PfnFoundStart + PfnFoundTotal ].ReferenceCount++;
            }

            *LogicalAddress = MmPfnDatabase[ PfnFoundStart ].PageFrameNumber << 12;
            *VirtualAddress = ( ULONG64 )MmMapIoSpaceSpecifyCache(
                MmPfnDatabase[ PfnFoundStart ].PageFrameNumber << 12,
                Length,
                AdapterObject->CacheMode );

            return STATUS_SUCCESS;
        }
    }

    while ( PfnFoundTotal-- ) {

        MmPfnDatabase[ PfnFoundStart + PfnFoundTotal ].LockBit = FALSE;
    }

    return STATUS_INSUFFICIENT_RESOURCES;
}

VOID
MmDmaFreeBuffer(
    _In_ PMM_DMA_ADAPTER AdapterObject,
    _In_ ULONG64         Length,
    _In_ ULONG64         LogicalAddress,
    _In_ ULONG64         VirtualAddress
)
{
    AdapterObject;

    PMM_PFN Pfn;
    ULONG64 PfnStart;
    ULONG64 PfnTotal;

    Length = ROUND_TO_PAGES( Length );

    Pfn = MmReferenceDatabaseByAddress( LogicalAddress );
    Pfn->LockBit = FALSE;
    Pfn->ReferenceCount--;

    PfnStart = Pfn - MmPfnDatabase;
    PfnTotal = Length >> 12;

    while ( PfnTotal-- ) {

        MmPfnDatabase[ PfnStart + PfnTotal ].LockBit = TRUE;
        MmChangePfnVaType( MmPfnDatabase + PfnStart + PfnTotal,
                           MmTypeModified );
        MmPfnDatabase[ PfnStart + PfnTotal ].ReferenceCount--;
        MmPfnDatabase[ PfnStart + PfnTotal ].LockBit = FALSE;
    }

    MmUnmapIoSpace( ( PVOID )VirtualAddress );
}

ULONG64
MmGetLogicalAddress(
    _In_ ULONG64 VirtualAddress
)
{
    ULONG64 PageOffset;

    PageOffset = VirtualAddress & 0xFFF;
    VirtualAddress &= ~0xFFF;

    return ( MiReferenceLevel2Entry( MiIndexLevel4( VirtualAddress ),
                                     MiIndexLevel3( VirtualAddress ),
                                     MiIndexLevel2( VirtualAddress ) )[ MiIndexLevel1( VirtualAddress ) ].PageFrameNumber << 12 ) + PageOffset;
}
