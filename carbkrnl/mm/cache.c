


#include <carbsup.h>
#include "mi.h"
#include "../hal/halp.h"

//
// first pcid is used by the kernel, poggers!
//

KSPIN_LOCK MiPcidLock = { 0 };
ULONG64    MiPcidLruCache[ 16 ] = { 1 };

ULONG64
MiAllocatePcid(

)
{
    ULONG64 CurrentPcid;
    ULONG64 LruPcid;
    ULONG64 UsePcid = ~0ull;
    KIRQL PreviousIrql;

    KeAcquireSpinLock( &MiPcidLock, &PreviousIrql );

    for ( CurrentPcid = 0; CurrentPcid < 16; CurrentPcid++ ) {

        if ( MiPcidLruCache[ CurrentPcid ] < UsePcid ) {

            LruPcid = CurrentPcid;
            UsePcid = MiPcidLruCache[ CurrentPcid ];
        }
    }

    MiPcidLruCache[ LruPcid ]++;
    KeReleaseSpinLock( &MiPcidLock, PreviousIrql );
    return LruPcid;
}

VOID
MiFreePcid(
    _In_ ULONG64 Pcid
)
{
    KIRQL PreviousIrql;
    KeAcquireSpinLock( &MiPcidLock, &PreviousIrql );
    MiPcidLruCache[ Pcid ]--;
    KeReleaseSpinLock( &MiPcidLock, PreviousIrql );
}

#if 0

// 
// Initial value of 1 for pcid 0, this is the kernel's pcid
//

KSPIN_LOCK MiPcidLock = { 0 };
ULONG64    MiPcidMap[ 64 ] = { 1 };

FORCEINLINE
VOID
MiSetPcidBit(
    _In_ ULONG64 Pcid,
    _In_ BOOLEAN Long
)
{
    if ( Long ) {

        MiPcidMap[ Pcid / 64 ] |= ( 1ull << ( Pcid % 64 ) );
    }
    else {

        MiPcidMap[ Pcid / 64 ] &= ~( 1ull << ( Pcid % 64 ) );
    }
}

FORCEINLINE
BOOLEAN
MiGetPcidBit(
    _In_ ULONG64 Pcid
)
{
    return ( MiPcidMap[ Pcid / 64 ] >> ( Pcid % 64 ) ) & 1;
}

ULONG64
MiAllocatePcid(

)
{
    ULONG64 CurrentPcid;
    KIRQL PreviousIrql;

    KeAcquireSpinLock( &MiPcidLock, &PreviousIrql );

    for ( CurrentPcid = 0; CurrentPcid < 4096; CurrentPcid++ ) {

        if ( !MiGetPcidBit( CurrentPcid ) ) {

            MiSetPcidBit( CurrentPcid, TRUE );
            KeReleaseSpinLock( &MiPcidLock, PreviousIrql );
            return CurrentPcid;
        }
    }

    //
    // what do we do when we run out of pcids?
    //
    // "least recently used" ?
    //

    KeBugCheck( STATUS_MEMORY_MANAGER );
}

VOID
MiFreePcid(
    _In_ ULONG64 Pcid
)
{
    KIRQL PreviousIrql;
    KeAcquireSpinLock( &MiPcidLock, &PreviousIrql );
    MiSetPcidBit( Pcid, FALSE );
    KeReleaseSpinLock( &MiPcidLock, PreviousIrql );
}

#endif

VOID
MmInitializeCaching(

)
{
    IA32_PAT_MSR Pat;

    //
    // These are the defaults but we need to write to the
    // pat msr to enable it anyways, so why not make sure 
    // they're set properly.
    //

    Pat.Long = __readmsr( IA32_MSR_PAT );
    Pat.Pa0 = MEM_TYPE_WB;
    Pat.Pa1 = MEM_TYPE_WC;
    Pat.Pa2 = MEM_TYPE_UC;
    __writemsr( IA32_MSR_PAT, Pat.Long );

    //
    // This layout means 
    // pat=0,cd=0,wt=0, pa0
    // pat=0,cd=0,wt=1, pa1
    // pat=0,cd=1,wt=0, pa2
    //

#if 0
    ULONG64 TypeCount;
    ULONG64 CurrentType;
    ULONG64 TypeBase;
    ULONG64 TypeMask;

    TypeCount = __readmsr( IA32_MTRR_CAPABILITIES ) & 0xFF;

    RtlDebugPrint( L"cap: %d\n", TypeCount );

    for ( CurrentType = 0; CurrentType < TypeCount; CurrentType++ ) {

        TypeBase = __readmsr( ( unsigned long )( IA32_MTRR_PHYSBASE0 + CurrentType * 2 ) );
        TypeMask = __readmsr( ( unsigned long )( IA32_MTRR_PHYSMASK0 + CurrentType * 2 ) );

        if ( TypeMask & ( 1 << 11 ) ) {

            RtlDebugPrint( L"s%d %ull %ull\n", CurrentType, TypeBase, TypeMask );
}
}
#endif

    //
    // These are the only features we utilize, if they exist
    // When the pcb is created, these bits are set appropriately.
    //

    if ( KeProcessorFeatureEnabled( NULL, KPF_NX_ENABLED ) ) {

        __writemsr( IA32_MSR_EFER, __readmsr( IA32_MSR_EFER ) | EFER_NXE );
    }

    if ( KeProcessorFeatureEnabled( NULL, KPF_PCID_ENABLED ) ) {

        __writecr4( __readcr4( ) | CR4_PCIDE );
    }

    if ( KeProcessorFeatureEnabled( NULL, KPF_SMEP_ENABLED ) ) {

        __writecr4( __readcr4( ) | CR4_SMEP );
    }
}
