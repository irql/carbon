/*++

Module ObjectName:

	gdt.c

Abstract:

	Global descriptor tables.

--*/

#include <carbsup.h>
#include "hal.h"
#include "mi.h"

/*
	these api's assume you wont use over a page of gdt entries,
	(over 512 entries including the null, windows only uses 128)

	a lot of apis were created to make expandability very easy.

	since we're x86_64 we can just ignore base and limits.
*/

VOID
HalGdtCreate(
	__inout PGDTR Gdtr
)
{

	Gdtr->Base = ( ULONG64 )MmAllocateMemory( 0x1000, PAGE_READ | PAGE_WRITE );

	_memset( ( void* )Gdtr->Base, 0, 0x1000 );
	Gdtr->Limit = 8 - 1;

	return;
}

USHORT
HalGdtAddEntry(
	__inout PGDTR Gdtr,
	__in PGLOBAL_DESCRIPTOR_TABLE_ENTRY GdtEntry
)
{

	USHORT Offset = Gdtr->Limit + 1;
	PVOID NextGdtEntry = ( PVOID )( ( PUCHAR )Gdtr->Base + Offset );

	*( ULONG64* )NextGdtEntry = *( ULONG64* )GdtEntry;

	Gdtr->Limit += sizeof( GLOBAL_DESCRIPTOR_TABLE_ENTRY );

	return Offset;
}

USHORT
HalGdtAddTss(
	__inout PGDTR Gdtr,
	__in PTSS Address,
	__in USHORT Size
)
{

	GDT_ENTRY_TSS TssEntry;
	ULONG64 u64Base = ( ULONG64 )Address;

	TssEntry.BaseLow = ( USHORT )u64Base;
	TssEntry.BaseMid = ( UCHAR )( u64Base >> 16 );
	TssEntry.BaseHigh = ( UCHAR )( u64Base >> 24 );
	TssEntry.BaseHigh32 = ( ULONG32 )( u64Base >> 32 );
	TssEntry.Reserved = 0;

	TssEntry.Length = Size;

	TssEntry.AccessByte = 0x89;
	TssEntry.Flags = 0;

	USHORT Tr = HalGdtAddEntry( Gdtr, ( PGLOBAL_DESCRIPTOR_TABLE_ENTRY )&TssEntry );
	HalGdtAddEntry( Gdtr, ( PGLOBAL_DESCRIPTOR_TABLE_ENTRY )&TssEntry + 1 );

	return Tr;
}