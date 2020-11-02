


#include <carbsup.h>
#include "mi.h"

PVOID
MmAllocateMemory(
	__in ULONG64 ByteCount,
	__in ULONG64 Flags
)
{

	PADDRESS_SPACE_DESCRIPTOR AddressSpace = MiGetAddressSpace( );

	ULONG64 EntryFlags = MiFlagsToEntryFlags( Flags );
	ULONG64 PageCount = ROUND_TO_PAGES( ByteCount ) / 0x1000;

	ULONG64 Address = MiAllocateVirtual( PageCount, Flags );

	USHORT Pml4tIndex, PdptIndex, PdtIndex, PtIndex;

	for ( ULONG64 i = 0; i < PageCount; i++ ) {

		MiVirtualToIndex( Address + ( i * 0x1000 ), &Pml4tIndex, &PdptIndex, &PdtIndex, &PtIndex );

		ULONG64* Pdpt = MiPageTableToVirtual( &AddressSpace->BaseVirtual[ Pml4tIndex ] );
		AddressSpace->BaseVirtual[ Pml4tIndex ] |= EntryFlags;

		ULONG64* Pdt = MiPageTableToVirtual( &Pdpt[ PdptIndex ] );
		Pdpt[ PdptIndex ] |= EntryFlags;

		ULONG64* Pt = MiPageTableToVirtual( &Pdt[ PdtIndex ] );
		Pdt[ PdtIndex ] |= EntryFlags;

		Pt[ PtIndex ] = MiAllocatePhysical( 1 ) | EntryFlags;
	}

	return ( PVOID )Address;
}

PVOID
MmAllocateMemoryAtPhysical(
	__in ULONG64 Physical,
	__in ULONG64 ByteCount,
	__in ULONG64 Flags
)
{

	PADDRESS_SPACE_DESCRIPTOR AddressSpace = MiGetAddressSpace( );

	ULONG64 EntryFlags = MiFlagsToEntryFlags( Flags );
	ULONG64 PageCount = ROUND_TO_PAGES( ByteCount ) / 0x1000;

	ULONG64 Address = MiAllocateVirtual( PageCount, Flags );

	USHORT Pml4tIndex, PdptIndex, PdtIndex, PtIndex;

	for ( ULONG64 i = 0; i < PageCount; i++ ) {

		MiVirtualToIndex( Address + ( i * 0x1000 ), &Pml4tIndex, &PdptIndex, &PdtIndex, &PtIndex );

		ULONG64* Pdpt = MiPageTableToVirtual( &AddressSpace->BaseVirtual[ Pml4tIndex ] );
		AddressSpace->BaseVirtual[ Pml4tIndex ] |= EntryFlags;

		ULONG64* Pdt = MiPageTableToVirtual( &Pdpt[ PdptIndex ] );
		Pdpt[ PdptIndex ] |= EntryFlags;

		ULONG64* Pt = MiPageTableToVirtual( &Pdt[ PdtIndex ] );
		Pdt[ PdtIndex ] |= EntryFlags;

		Pt[ PtIndex ] = ( Physical + ( i * 0x1000 ) ) | EntryFlags;
	}

	return ( PVOID )Address;
}

PVOID
MmAllocateMemoryAtVirtual(
	__in ULONG64 Virtual,
	__in ULONG64 ByteCount,
	__in ULONG64 Flags
)
{
	// should this really overwrite?
	// no it fucking shouldn't

	PADDRESS_SPACE_DESCRIPTOR AddressSpace = MiGetAddressSpace( );

	ULONG64 EntryFlags = MiFlagsToEntryFlags( Flags );
	ULONG64 PageCount = ROUND_TO_PAGES( ByteCount ) / 0x1000;

	Virtual = MiAllocateVirtualAt( Virtual, PageCount, Flags );

	USHORT Pml4tIndex, PdptIndex, PdtIndex, PtIndex;

	for ( ULONG64 i = 0; i < PageCount; i++ ) {

		MiVirtualToIndex( Virtual + ( i * 0x1000 ), &Pml4tIndex, &PdptIndex, &PdtIndex, &PtIndex );

		ULONG64* Pdpt = MiPageTableToVirtual( &AddressSpace->BaseVirtual[ Pml4tIndex ] );
		AddressSpace->BaseVirtual[ Pml4tIndex ] |= EntryFlags;

		ULONG64* Pdt = MiPageTableToVirtual( &Pdpt[ PdptIndex ] );
		Pdpt[ PdptIndex ] |= EntryFlags;

		ULONG64* Pt = MiPageTableToVirtual( &Pdt[ PdtIndex ] );
		Pdt[ PdtIndex ] |= EntryFlags;

		Pt[ PtIndex ] = MiAllocatePhysical( 1 ) | EntryFlags;
	}

	return ( PVOID )Virtual;
}

//This could return NTSTATUS, but its kernel mode and can be forced.

VOID
MmAllocateMemoryAt(
	__in ULONG64 Virtual,
	__in ULONG64 Physical,
	__in ULONG64 ByteCount,
	__in ULONG64 Flags
)
{

	PADDRESS_SPACE_DESCRIPTOR AddressSpace = MiGetAddressSpace( );

	ULONG64 EntryFlags = MiFlagsToEntryFlags( Flags );
	ULONG64 PageCount = ROUND_TO_PAGES( ByteCount ) / 0x1000;

	USHORT Pml4tIndex, PdptIndex, PdtIndex, PtIndex;

	for ( ULONG64 i = 0; i < PageCount; i++ ) {

		MiVirtualToIndex( Virtual + ( i * 0x1000 ), &Pml4tIndex, &PdptIndex, &PdtIndex, &PtIndex );

		ULONG64* Pdpt = MiPageTableToVirtual( &AddressSpace->BaseVirtual[ Pml4tIndex ] );
		AddressSpace->BaseVirtual[ Pml4tIndex ] |= EntryFlags;

		ULONG64* Pdt = MiPageTableToVirtual( &Pdpt[ PdptIndex ] );
		Pdpt[ PdptIndex ] |= EntryFlags;

		ULONG64* Pt = MiPageTableToVirtual( &Pdt[ PdtIndex ] );
		Pdt[ PdtIndex ] |= EntryFlags;

		Pt[ PtIndex ] = ( Physical + ( i * 0x1000 ) ) | EntryFlags;
	}
}

VOID
MmFreeMemory(
	__in ULONG64 Address,
	__in ULONG64 ByteCount
)
{

	PADDRESS_SPACE_DESCRIPTOR AddressSpace = MiGetAddressSpace( );

	ULONG64 PageCount = ROUND_TO_PAGES( ByteCount ) / 0x1000;

	USHORT Pml4tIndex, PdptIndex, PdtIndex, PtIndex;

	for ( ULONG64 i = 0; i < PageCount; i++ ) {

		MiVirtualToIndex( Address + ( i * 0x1000 ), &Pml4tIndex, &PdptIndex, &PdtIndex, &PtIndex );

		if ( !( AddressSpace->BaseVirtual[ Pml4tIndex ] & EntryPresent ) ) {

			return;
		}

		ULONG64* Pdpt = MiPageTableToVirtual( &AddressSpace->BaseVirtual[ Pml4tIndex ] );

		if ( !( Pdpt[ PdptIndex ] & EntryPresent ) ) {

			return;
		}

		ULONG64* Pdt = MiPageTableToVirtual( &Pdpt[ PdptIndex ] );

		if ( !( Pdt[ PdtIndex ] & EntryPresent ) ) {

			return;
		}

		ULONG64* Pt = MiPageTableToVirtual( &Pdt[ PdtIndex ] );

		if ( !( Pt[ PtIndex ] & EntryPresent ) ) {

			return;
		}

		MiMarkPhysical( Pt[ PtIndex ] & ~0xfff, 1, FALSE );
		Pt[ PtIndex ] = 0;
		MiInvalidateTlbEntry( Address + ( i * 0x1000 ) );
	}

}
