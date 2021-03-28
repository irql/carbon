


#include <carbsup.h>
#include "halp.h"

VOID
HalCreateGlobal(
    _Inout_ PKDESCRIPTOR_TABLE Gdtr
)
{
    Gdtr->Base = ( ULONG64 )MmAllocatePoolWithTag( NonPagedPoolZeroed, 0x1000, HAL_TAG );
    Gdtr->Limit = 8 - 1;
}

ULONG
HalInsertCodeSegment(
    _Inout_ PKDESCRIPTOR_TABLE  Gdtr,
    _In_    PKGDT_CODE_SEGMENT Entry
)
{
    ULONG Offset;
    PULONG64 Next;

    Offset = Gdtr->Limit + 1;
    Next = ( PULONG64 )( Gdtr->Base + Offset );
    *Next = Entry->Long;
    Gdtr->Limit += sizeof( KGDT_CODE_SEGMENT );
    return Offset;
}

ULONG
HalInsertSystemSegment(
    _Inout_ PKDESCRIPTOR_TABLE  Gdtr,
    _In_    PKGDT_SYSTEM_SEGMENT Entry
)
{
    ULONG Offset;
    PULONG64 Next;

    Offset = Gdtr->Limit + 1;
    Next = ( PULONG64 )( Gdtr->Base + Offset );
    *Next++ = Entry->Long0;
    *Next++ = Entry->Long1;
    Gdtr->Limit += sizeof( KGDT_SYSTEM_SEGMENT );
    return Offset;
}

ULONG
HalInsertTaskSegment(
    _Inout_ PKDESCRIPTOR_TABLE Gdtr,
    _In_    PKTASK_STATE   Task,
    _In_    ULONG          Length
)
{
    ULONG TaskRegister;
    KGDT_SYSTEM_SEGMENT TaskSegment;
    ULONG64 Base = ( ULONG64 )Task;

    TaskSegment.Long0 = 0;
    TaskSegment.Long1 = 0;
    TaskSegment.BaseLow = ( USHORT )( Base );
    TaskSegment.BaseMid = ( UCHAR )( Base >> 16 );
    TaskSegment.BaseHigh = ( UCHAR )( Base >> 24 );
    TaskSegment.BaseUpper = ( ULONG )( Base >> 32 );
    TaskSegment.Type = SYSTEM_SEGMENT_TYPE_TSS;
    TaskSegment.Present = 1;
    TaskSegment.LimitLow = Length - 1;

    TaskRegister = HalInsertSystemSegment( Gdtr, &TaskSegment );

    return TaskRegister;
}

VOID
HalSetCodeSegmentBase(
    _Inout_ PKGDT_CODE_SEGMENT Entry,
    _In_    PVOID           Base
)
{
    ULONG64 BaseAddress;

    BaseAddress = ( ULONG64 )Base;

    Entry->BaseLow = ( USHORT )( BaseAddress );
    Entry->BaseMid = ( UCHAR )( BaseAddress >> 16 );
    Entry->BaseHigh = ( UCHAR )( BaseAddress >> 24 );
    //Entry->BaseUpper = ( ULONG )( BaseAddress >> 32 );
}
