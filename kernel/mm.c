


#include <carbsup.h>
#include "mi.h"

PVOID MmAllocateMemory(
	__in ULONG64 ByteCount,
	__in ULONG64 Flags
) {
	PADDRESS_SPACE_DESCRIPTOR AddressSpace = MiGetAddressSpace( );

	ULONG64 EntryFlags = MiFlagsToEntryFlags( Flags );
	ULONG64 Pages = ROUND_TO_PAGES( ByteCount ) / PAGE_SIZE;

	ULONG64 Address = MiAllocateVirtual( Pages, Flags );
	USHORT	Pml4tIndex, PdptIndex, PdtIndex, PtIndex;

	while ( Pages-- ) {
		MiVirtualToIndex( Address + Pages * PAGE_SIZE, &Pml4tIndex, &PdptIndex, &PdtIndex, &PtIndex );

		ULONG64 *Pdpt = MiPageTableToVirtual( &AddressSpace->BaseVirtual[ Pml4tIndex ] );
		AddressSpace->BaseVirtual[ Pml4tIndex ] |= EntryFlags;

		ULONG64 *Pdt = MiPageTableToVirtual( &Pdpt[ PdptIndex ] );
		Pdpt[ PdptIndex ] |= EntryFlags;

		ULONG64 *Pt = MiPageTableToVirtual( &Pdt[ PdtIndex ] );
		Pdt[ PdtIndex ] |= EntryFlags;

		Pt[ PtIndex ] = MiAllocatePhysical( 1 ) | EntryFlags;
	}

	return ( PVOID )Address;
}

PVOID MmAllocateMemoryAtPhysical(
	__in ULONG64 Physical,
	__in ULONG64 ByteCount,
	__in ULONG64 Flags
) {
	PADDRESS_SPACE_DESCRIPTOR AddressSpace = MiGetAddressSpace( );

	ULONG64 EntryFlags = MiFlagsToEntryFlags( Flags );
	ULONG64 Pages = ROUND_TO_PAGES( ByteCount ) / PAGE_SIZE;

	ULONG64 Address = MiAllocateVirtual( Pages, Flags );
	USHORT	Pml4tIndex, PdptIndex, PdtIndex, PtIndex;

	while ( Pages-- ) {
		MiVirtualToIndex( Address + Pages * PAGE_SIZE, &Pml4tIndex, &PdptIndex, &PdtIndex, &PtIndex );

		ULONG64* Pdpt = MiPageTableToVirtual( &AddressSpace->BaseVirtual[ Pml4tIndex ] );
		AddressSpace->BaseVirtual[ Pml4tIndex ] |= EntryFlags;

		ULONG64* Pdt = MiPageTableToVirtual( &Pdpt[ PdptIndex ] );
		Pdpt[ PdptIndex ] |= EntryFlags;

		ULONG64* Pt = MiPageTableToVirtual( &Pdt[ PdtIndex ] );
		Pdt[ PdtIndex ] |= EntryFlags;

		Pt[ PtIndex ] = ( Physical + Pages * PAGE_SIZE) | EntryFlags;
	}

	return ( PVOID )Address;
}

PVOID MmAllocateMemoryAtVirtual(
	__in ULONG64 Virtual,
	__in ULONG64 ByteCount,
	__in ULONG64 Flags
) {
	// should this really overwrite?
	// no it fucking shouldn't

	PADDRESS_SPACE_DESCRIPTOR AddressSpace = MiGetAddressSpace( );

	ULONG64 EntryFlags = MiFlagsToEntryFlags( Flags );
	ULONG64 Pages = ROUND_TO_PAGES( ByteCount ) / PAGE_SIZE;

	USHORT Pml4tIndex, PdptIndex, PdtIndex, PtIndex;

	Virtual = MiAllocateVirtualAt(Virtual, Pages, Flags);

	while ( Pages-- ) {
		MiVirtualToIndex( Virtual + Pages * PAGE_SIZE, &Pml4tIndex, &PdptIndex, &PdtIndex, &PtIndex );

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

VOID MmAllocateMemoryAt(
	__in ULONG64 Virtual,
	__in ULONG64 Physical,
	__in ULONG64 ByteCount,
	__in ULONG64 Flags
) {

	PADDRESS_SPACE_DESCRIPTOR AddressSpace = MiGetAddressSpace( );

	ULONG64 EntryFlags = MiFlagsToEntryFlags( Flags );
	ULONG64 Pages = ROUND_TO_PAGES( ByteCount ) / 0x1000;

	USHORT Pml4tIndex, PdptIndex, PdtIndex, PtIndex;

	while ( Pages-- ) {

		MiVirtualToIndex( Virtual + Pages * PAGE_SIZE, &Pml4tIndex, &PdptIndex, &PdtIndex, &PtIndex );

		ULONG64* Pdpt = MiPageTableToVirtual( &AddressSpace->BaseVirtual[ Pml4tIndex ] );
		AddressSpace->BaseVirtual[ Pml4tIndex ] |= EntryFlags;

		ULONG64* Pdt = MiPageTableToVirtual( &Pdpt[ PdptIndex ] );
		Pdpt[ PdptIndex ] |= EntryFlags;

		ULONG64* Pt = MiPageTableToVirtual( &Pdt[ PdtIndex ] );
		Pdt[ PdtIndex ] |= EntryFlags;

		Pt[ PtIndex ] = ( Physical + Pages * PAGE_SIZE ) | EntryFlags;
	}
}

VOID MmFreeMemory(
	__in ULONG64 Address,
	__in ULONG64 ByteCount
) {
	PADDRESS_SPACE_DESCRIPTOR AddressSpace = MiGetAddressSpace( );

	ULONG64 Pages = ROUND_TO_PAGES( ByteCount ) / PAGE_SIZE;
	USHORT Pml4tIndex, PdptIndex, PdtIndex, PtIndex;

	while ( Pages-- ) {
		MiVirtualToIndex( Address + Pages * PAGE_SIZE, &Pml4tIndex, &PdptIndex, &PdtIndex, &PtIndex );
		if ( ( AddressSpace->BaseVirtual[ Pml4tIndex ] & EntryPresent ) == 0 ) return;

		ULONG64 *Pdpt = MiPageTableToVirtual( &AddressSpace->BaseVirtual[ Pml4tIndex ] );
		if ( ( Pdpt[ PdptIndex ] & EntryPresent ) == 0 ) return;

		ULONG64 *Pdt = MiPageTableToVirtual( &Pdpt[ PdptIndex ] );
		if ( ( Pdt[ PdtIndex ] & EntryPresent ) == 0 ) return;

		ULONG64 *Pt = MiPageTableToVirtual( &Pdt[ PdtIndex ] );
		if ( ( Pt[ PtIndex ] & EntryPresent ) == 0 ) return;

		MiMarkPhysical( Pt[ PtIndex ] & ~0x0FFF, 1, FALSE );
		Pt[ PtIndex ] = 0;

		MiInvalidateTlbEntry( Address + Pages * PAGE_SIZE);
	}
}
