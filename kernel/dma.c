


#include <carbsup.h>
#include "mm.h"

EXTERN PLIST_ENTRY PhysicalRegionHead;

VOID
MmAllocateDmaMemory(
	__inout PDMA_CHUNK DmaChunk
)
{
	PADDRESS_SPACE_DESCRIPTOR AddressSpace = MiGetAddressSpace( );

	PLIST_ENTRY Flink = PhysicalRegionHead;
	do {
		PPHYSICAL_REGION_DESCRIPTOR RegionDescriptor = CONTAINING_RECORD( Flink, PHYSICAL_REGION_DESCRIPTOR, RegionLinks );

		KeAcquireSpinLock( &RegionDescriptor->RegionLock );

		ULONG64 FoundIndex = ( ULONG64 )-1;
		ULONG64 FoundCount = 0;

		for ( ULONG64 i = 0; i < ( RegionDescriptor->RegionLength / 0x1000 ); ) {

			if ( RegionDescriptor->Bitmap[ i / 64 ] == ( ULONG64 )-1 ) {

				i += 64;
				FoundIndex = ( ULONG64 )-1;
				FoundCount = 0;
				continue;
			}

			if ( ( RegionDescriptor->Bitmap[ i / 64 ] & ( 1i64 << ( i % 64 ) ) ) == 0 ) {

				if ( FoundIndex == ( ULONG64 )-1 ) {

					FoundIndex = i;
				}

				FoundCount++;

				if ( FoundCount == DmaChunk->PageCount ) {

					i = FoundIndex;
					for ( ULONG64 j = 0; j < DmaChunk->PageCount; j++, i++ ) {

						RegionDescriptor->Bitmap[ i / 64 ] |= ( 1i64 << ( i % 64 ) );
					}

					KeReleaseSpinLock( &RegionDescriptor->RegionLock );

					DmaChunk->BasePhysical = RegionDescriptor->RegionBase + FoundIndex * 0x1000;
					goto Found;
				}

				i++;
			}
			else {

				FoundIndex = ( ULONG64 )-1;
				FoundCount = 0;
				i += 16;
				i = ( i / 16 ) * 16;
			}
		}

		KeReleaseSpinLock( &RegionDescriptor->RegionLock );

		Flink = Flink->Flink;
	} while ( Flink != PhysicalRegionHead );

	/* panic. */

	__halt( );

Found:;

	DmaChunk->BaseVirtual = MiAllocateVirtual( DmaChunk->PageCount, EntryPresent | EntryWriteable );

	USHORT Pml4tIndex, PdptIndex, PdtIndex, PtIndex;

	for ( ULONG64 i = 0; i < DmaChunk->PageCount; i++ ) {

		MiVirtualToIndex( DmaChunk->BaseVirtual + ( i * 0x1000 ), &Pml4tIndex, &PdptIndex, &PdtIndex, &PtIndex );

		ULONG64* Pdpt = MiPageTableToVirtual( &AddressSpace->BaseVirtual[ Pml4tIndex ] );
		AddressSpace->BaseVirtual[ Pml4tIndex ] |= EntryPresent | EntryWriteable;

		ULONG64* Pdt = MiPageTableToVirtual( &Pdpt[ PdptIndex ] );
		Pdpt[ PdptIndex ] |= EntryPresent | EntryWriteable;

		ULONG64* Pt = MiPageTableToVirtual( &Pdt[ PdtIndex ] );
		Pdt[ PdtIndex ] |= EntryPresent | EntryWriteable;

		Pt[ PtIndex ] = ( DmaChunk->BasePhysical + ( i * 0x1000 ) ) | ( EntryPresent | EntryWriteable );
	}
}


PVOID
MmAllocateContiguousMemory(
	__in ULONG64 ByteCount,
	__in ULONG64 Flags
)
{
	PADDRESS_SPACE_DESCRIPTOR AddressSpace = MiGetAddressSpace( );

	ULONG64 PageCount = ROUND_TO_PAGES( ByteCount ) / 0x1000;

	ULONG64 EntryFlags = MiFlagsToEntryFlags( Flags );

	ULONG64 Physical = MiAllocatePhysical( PageCount );
	ULONG64 Virtual = MiAllocateVirtual( PageCount, Flags );

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

	return ( PVOID )Virtual;
}