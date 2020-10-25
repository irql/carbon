


#include <carbsup.h>
#include "mm.h"

KLOCKED_LIST AddressSpaceHead = { 0 };
ADDRESS_SPACE_DESCRIPTOR KernelPageTable = { 0 };

PVOID
MiPageTableToVirtual(
	__in PULONG64 PageTable
)
{
	/* responsible for searching and creating (that's why ulong64*) */

	PADDRESS_SPACE_DESCRIPTOR AddressSpace = MiGetAddressSpace( );

	ULONG64 VirtualAddress = 0;

	if ( *PageTable & EntryPresent ) {
		/* exists */

		ULONG64 PageTableFrame = *PageTable & EntryFrame;

		PLIST_ENTRY Flink = AddressSpace->PageTableAllocationHead;
		do {
			PPAGE_TABLE_ALLOCATION_ENTRY AllocationEntry = CONTAINING_RECORD( Flink, PAGE_TABLE_ALLOCATION_ENTRY, AllocationLinks );

			KeAcquireSpinLock( &AllocationEntry->AllocationLock );

			if ( PageTableFrame >= AllocationEntry->BasePhysical &&
				PageTableFrame < ( AllocationEntry->BasePhysical + 0x200000 ) ) {

				VirtualAddress = PageTableFrame - AllocationEntry->BasePhysical;
				VirtualAddress += AllocationEntry->BaseVirtual;

				KeReleaseSpinLock( &AllocationEntry->AllocationLock );

				return ( PVOID )VirtualAddress;
			}

			KeReleaseSpinLock( &AllocationEntry->AllocationLock );

			Flink = Flink->Flink;
		} while ( Flink != AddressSpace->PageTableAllocationHead );


		//Check The BUG.

		return 0;
	}
	else {

		PLIST_ENTRY Flink = AddressSpace->PageTableAllocationHead;
		do {
			PPAGE_TABLE_ALLOCATION_ENTRY AllocationEntry = CONTAINING_RECORD( Flink, PAGE_TABLE_ALLOCATION_ENTRY, AllocationLinks );

			KeAcquireSpinLock( &AllocationEntry->AllocationLock );

			/*
				starts at 1 because the allocation entry occupies the first page.
			*/

			for ( ULONG32 i = 1; i < 512; i++ ) {

				if ( AllocationEntry->Bitmap[ i / 64 ] == ( ULONG64 )-1 ) {

					i += 63;
					continue;
				}

				if ( ( AllocationEntry->Bitmap[ i / 64 ] & ( 1i64 << ( i % 64 ) ) ) == 0 ) {

					*PageTable = ( AllocationEntry->BasePhysical + ( i * 0x1000 ) ) | EntryPresent;
					VirtualAddress = AllocationEntry->BaseVirtual + ( i * 0x1000 );
					AllocationEntry->Bitmap[ i / 64 ] |= ( 1i64 << ( i % 64 ) );

					break;
				}
			}

			KeReleaseSpinLock( &AllocationEntry->AllocationLock );

			if ( *PageTable & EntryPresent ) {

				break;
			}

			Flink = Flink->Flink;
		} while ( Flink != AddressSpace->PageTableAllocationHead );

		/*
			this checks how many more page tables we can allocate,
			this is so that if we need more, we allocate a new one using the remaining.

			counts the bits remaining (1 bit = 0x1000)
			if its == to 4 then we should create a new one.
		*/

		PPAGE_TABLE_ALLOCATION_ENTRY AllocationEntry = CONTAINING_RECORD( AddressSpace->PageTableAllocationHead->Blink, PAGE_TABLE_ALLOCATION_ENTRY, AllocationLinks );
		ULONG32 BitCount = 0;

		ULONG32 BitPositions[ 3 ] = { 0, 0, 0 };

		for ( ULONG32 i = 0, j = 0; i < 512; i++ ) {

			if ( AllocationEntry->Bitmap[ i / 64 ] == 0i64 ) {

				return ( PVOID )VirtualAddress;
			}

			if ( ( AllocationEntry->Bitmap[ i / 64 ] & ( 1i64 << ( i % 64 ) ) ) == 0 ) {

				BitCount++;

				if ( BitCount > 3 ) {

					return ( PVOID )VirtualAddress;
				}

				BitPositions[ j++ ] = i;
			}
		}

		if ( BitCount == 3 ) {

			ULONG64 BasePhysical = MiAllocatePhysical( 512 );
			ULONG64 BaseVirtual;
			//ULONG64 BaseVirtual = MiAllocateVirtual( 512, EntryPresent | EntryWriteable ); 
			//this can cause a stack overflow and anal the system so we just rewrite it here taking the parts we need

			KeAcquireSpinLock( &AddressSpace->TableLock );

			ULONG64* Pml4t = AddressSpace->BaseVirtual;

			USHORT Pml4tIndex = 256, PdptIndex = 0, PdtIndex = 0;

			for ( ; Pml4tIndex < 512; Pml4tIndex++ ) {

				if ( Pml4t[ Pml4tIndex ] & EntryPresent ) {

					ULONG64* Pdpt = ( ULONG64* )MiPageTableToVirtual( &Pml4t[ Pml4tIndex ] );

					for ( ; PdptIndex < 512; PdptIndex++ ) {

						if ( Pdpt[ PdptIndex ] & EntryPresent ) {

							ULONG64* Pdt = ( ULONG64* )MiPageTableToVirtual( &Pdpt[ PdptIndex ] );

							for ( ; PdtIndex < 512; PdtIndex++ ) {

								if ( !( Pdt[ PdtIndex ] & EntryPresent ) ) {

									BaseVirtual = MiIndexToVirtual( Pml4tIndex, PdptIndex, PdtIndex, 0 );
									goto Found;
								}
							}
						}
						else {

							BaseVirtual = MiIndexToVirtual( Pml4tIndex, PdptIndex, 0, 0 );
							goto Found;
						}
					}
				}
				else {

					BaseVirtual = MiIndexToVirtual( Pml4tIndex, 0, 0, 0 );
					goto Found;
				}
			}

		Found:;

			if ( !( Pml4t[ Pml4tIndex ] & EntryPresent ) ) {
				Pml4t[ Pml4tIndex ] = ( AllocationEntry->BasePhysical + ( ( ULONG64 )BitPositions[ 0 ] * 0x1000 ) ) | EntryPresent | EntryWriteable;
				AllocationEntry->Bitmap[ BitPositions[ 0 ] / 64 ] |= ( 1i64 << ( BitPositions[ 0 ] % 64 ) );
			}

			ULONG64* Pdpt = ( ULONG64* )( Pml4t[ Pml4tIndex ] & ~0xFFF );

			if ( !( Pdpt[ PdptIndex ] & EntryPresent ) ) {
				Pdpt[ PdptIndex ] = ( AllocationEntry->BasePhysical + ( ( ULONG64 )BitPositions[ 1 ] * 0x1000 ) ) | EntryPresent | EntryWriteable;
				AllocationEntry->Bitmap[ BitPositions[ 1 ] / 64 ] |= ( 1i64 << ( BitPositions[ 1 ] % 64 ) );
			}

			ULONG64* Pdt = ( ULONG64* )( Pdpt[ PdptIndex ] & ~0xFFF );

			if ( !( Pdt[ PdtIndex ] & EntryPresent ) ) {
				Pdt[ PdtIndex ] = ( AllocationEntry->BasePhysical + ( ( ULONG64 )BitPositions[ 2 ] * 0x1000 ) ) | EntryPresent | EntryWriteable;
				AllocationEntry->Bitmap[ BitPositions[ 2 ] / 64 ] |= ( 1i64 << ( BitPositions[ 2 ] % 64 ) );
			}

			ULONG64* Pt = ( ULONG64* )( Pdt[ PdtIndex ] & ~0xFFF );

			for ( ULONG64 i = 0; i < 512; i++ ) {

				Pt[ i ] = ( BasePhysical + i * 0x1000 ) | EntryPresent | EntryWriteable;
			}

			KeReleaseSpinLock( &AddressSpace->TableLock );

			return ( PVOID )VirtualAddress;
		}

		return ( PVOID )VirtualAddress;
	}
}

PADDRESS_SPACE_DESCRIPTOR
MiGetAddressSpace(

)
{

	ULONG64 BasePhysical = __readcr3( );

	if ( BasePhysical == KernelPageTable.BasePhysical ) {

		return &KernelPageTable;
	}

	MiSetAddressSpace( &KernelPageTable );

	KeAcquireSpinLock( &AddressSpaceHead.Lock );

	PLIST_ENTRY Flink = AddressSpaceHead.List;
	do {
		PADDRESS_SPACE_DESCRIPTOR AddressSpace = CONTAINING_RECORD( Flink, ADDRESS_SPACE_DESCRIPTOR, TableLinks );

		if ( AddressSpace->BasePhysical == BasePhysical ) {

			KeReleaseSpinLock( &AddressSpaceHead.Lock );

			MiSetAddressSpace( AddressSpace );
			return AddressSpace;
		}

		Flink = Flink->Flink;
	} while ( Flink != AddressSpaceHead.List );

	/* explode. */

	return NULL;
}

VOID
MiSetAddressSpace(
	__in PADDRESS_SPACE_DESCRIPTOR AddressSpace
)
{

	__writecr3( AddressSpace->BasePhysical ); // this reminds me, make the kernel global or you can just page fault here.

}

VOID
MiEnterKernelSpace(
	__out PADDRESS_SPACE_DESCRIPTOR *PreviousAddressSpace
)
{

	*PreviousAddressSpace = MiGetAddressSpace( );

	MiSetAddressSpace( &KernelPageTable );

}

VOID
MiLeaveKernelSpace(
	__in PADDRESS_SPACE_DESCRIPTOR PreviousAddressSpace
)
{

	MiSetAddressSpace( PreviousAddressSpace );
}


ULONG64
MmPhysicalMapping(
	__in ULONG64 Address
)
{

	ULONG64 Offset = Address & 0xfff;
	Address &= ~0xfff;
	Address &= ~0xffff000000000000;

	USHORT Pml4tIndex, PdptIndex, PdtIndex, PtIndex;

	MiVirtualToIndex( Address, &Pml4tIndex, &PdptIndex, &PdtIndex, &PtIndex );

	PADDRESS_SPACE_DESCRIPTOR AddressSpace = MiGetAddressSpace( );

	ULONG64* Pdpt = MiPageTableToVirtual( &AddressSpace->BaseVirtual[ Pml4tIndex ] );
	ULONG64* Pdt = MiPageTableToVirtual( &Pdpt[ PdptIndex ] );
	ULONG64* Pt = MiPageTableToVirtual( &Pdt[ PdtIndex ] );

	return ( Pt[ PtIndex ] & ~0xfff ) + Offset;

}
