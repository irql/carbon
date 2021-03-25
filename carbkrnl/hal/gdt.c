


#include <carbsup.h>
#include "halp.h"

VOID
HalGdtCreate(
    _Inout_ PKSEG_DESC_REG Gdtr
)
{
    Gdtr->Base = ( ULONG64 )MmAllocatePoolWithTag( NonPagedPoolZeroed, 0x1000, HAL_TAG );
    Gdtr->Limit = 8 - 1;
}

ULONG
HalGdtAddSegEntry(
    _Inout_ PKSEG_DESC_REG  Gdtr,
    _In_    PKGDT_SEG_ENTRY Entry
)
{
    ULONG Offset;
    PULONG64 Next;

    Offset = Gdtr->Limit + 1;
    Next = ( PULONG64 )( Gdtr->Base + Offset );
    *Next = Entry->Long;
    Gdtr->Limit += sizeof( KGDT_SEG_ENTRY );
    return Offset;
}

ULONG
HalGdtAddAltEntry(
    _Inout_ PKSEG_DESC_REG  Gdtr,
    _In_    PKGDT_ALT_ENTRY Entry
)
{
    ULONG Offset;
    PULONG64 Next;

    Offset = Gdtr->Limit + 1;
    Next = ( PULONG64 )( Gdtr->Base + Offset );
    *Next++ = Entry->Value0;
    *Next++ = Entry->Value1;
    Gdtr->Limit += sizeof( KGDT_ALT_ENTRY );
    return Offset;
}

ULONG
HalGdtAddTss(
    _Inout_ PKSEG_DESC_REG Gdtr,
    _In_    PKTASK_STATE   Task,
    _In_    ULONG          Length
)
{
    ULONG TaskRegister;
    KGDT_ALT_ENTRY TaskEntry;
    ULONG64 Base = ( ULONG64 )Task;

    TaskEntry.Value0 = 0;
    TaskEntry.Value1 = 0;
    TaskEntry.BaseLow = ( USHORT )( Base );
    TaskEntry.BaseMid = ( UCHAR )( Base >> 16 );
    TaskEntry.BaseHigh = ( UCHAR )( Base >> 24 );
    TaskEntry.BaseUpper = ( ULONG )( Base >> 32 );
    TaskEntry.Accessed = 1;
    TaskEntry.Executable = 1;
    TaskEntry.Present = 1;
    TaskEntry.LimitLow = Length;

    TaskRegister = HalGdtAddAltEntry( Gdtr, &TaskEntry );

    return TaskRegister;
}

VOID
HalGdtSetSegBase(
    _Inout_ PKGDT_SEG_ENTRY Entry,
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
