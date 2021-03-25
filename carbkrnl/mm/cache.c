


#include <carbsup.h>
#include "mi.h"

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
    // We also should setup pcids here
    //

    //__writecr4( __readcr4( ) | ( 1 << 17 ) );
}
