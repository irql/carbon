


#include <carbsup.h>
#include "mm.h"

ULONG64
MiAllocateVirtual(
	__in ULONG64 PageCount,
	__in ULONG64 Flags
)
{
	PADDRESS_SPACE_DESCRIPTOR AddressSpace = MiGetAddressSpace( );

	KeAcquireSpinLock( &AddressSpace->TableLock );

	ULONG64* Pml4t = AddressSpace->BaseVirtual;

	ULONG64 FoundCount = PageCount;
	ULONG64 FoundVirtual = 0;

	for ( USHORT i = !( Flags & PAGE_USER ) * 256; i < 512; i++ ) {

		if ( Pml4t[ i ] & EntryPresent ) {

			ULONG64* Pdpt = ( ULONG64* )MiPageTableToVirtual( &Pml4t[ i ] );

			for ( USHORT j = 0; j < 512; j++ ) {

				if ( Pdpt[ j ] & EntryPresent ) {

					ULONG64* Pdt = ( ULONG64* )MiPageTableToVirtual( &Pdpt[ j ] );

					for ( USHORT k = 0; k < 512; k++ ) {

						if ( Pdt[ k ] & EntryPresent ) {

							ULONG64* Pt = ( ULONG64* )MiPageTableToVirtual( &Pdt[ k ] );

							for ( USHORT l = 0; l < 512; l++ ) {

								if ( Pt[ l ] & EntryPresent ) {

									FoundCount = PageCount;
									FoundVirtual = 0;
								}
								else {

									if ( FoundVirtual == 0 ) {

										FoundVirtual = MiIndexToVirtual( i, j, k, l );
									}

									FoundCount--;

									if ( FoundCount == 0 ) {

										MiMarkVirtual( FoundVirtual, PageCount, Flags );

										KeReleaseSpinLock( &AddressSpace->TableLock );

										return FoundVirtual;
									}
								}
							}
						}
						else {

							if ( FoundVirtual == 0 ) {

								FoundVirtual = MiIndexToVirtual( i, j, k, 0 );
							}

							if ( FoundCount <= ( 512 ) ) {

								MiMarkVirtual( FoundVirtual, PageCount, Flags );

								KeReleaseSpinLock( &AddressSpace->TableLock );

								return FoundVirtual;
							}
							else {

								FoundCount -= ( 512 );
							}
						}
					}
				}
				else {

					if ( FoundVirtual == 0 ) {

						FoundVirtual = MiIndexToVirtual( i, j, 0, 0 );
					}

					if ( FoundCount <= ( 512 * 512 ) ) {

						MiMarkVirtual( FoundVirtual, PageCount, Flags );

						KeReleaseSpinLock( &AddressSpace->TableLock );

						return FoundVirtual;
					}
					else {

						FoundCount -= ( 512 * 512 );
					}
				}
			}
		}
		else {

			if ( FoundVirtual == 0 ) {

				FoundVirtual = MiIndexToVirtual( i, 0, 0, 0 );
			}

			if ( FoundCount <= ( 512 * 512 * 512 ) ) {

				MiMarkVirtual( FoundVirtual, PageCount, Flags );

				KeReleaseSpinLock( &AddressSpace->TableLock );

				return FoundVirtual;
			}
			else {

				FoundCount -= ( 512 * 512 * 512 );
			}
		}
	}

	KeReleaseSpinLock( &AddressSpace->TableLock );

	/* Bug my cock Out Immediately. */

	return 0;
}

VOID
MiMarkVirtual(
	__in ULONG64 Address,
	__in ULONG64 PageCount,
	__in ULONG64 Flags
)
{
	PADDRESS_SPACE_DESCRIPTOR AddressSpace = MiGetAddressSpace( );

	USHORT Pml4tIndex, PdptIndex, PdtIndex, PtIndex;

	for ( ULONG64 i = 0; i < PageCount; i++ ) {

		MiVirtualToIndex( Address + ( i * 0x1000 ), &Pml4tIndex, &PdptIndex, &PdtIndex, &PtIndex );

		ULONG64* Pdpt = MiPageTableToVirtual( &AddressSpace->BaseVirtual[ Pml4tIndex ] );
		AddressSpace->BaseVirtual[ Pml4tIndex ] |= Flags;

		ULONG64* Pdt = MiPageTableToVirtual( &Pdpt[ PdptIndex ] );
		Pdpt[ PdptIndex ] |= Flags;

		ULONG64* Pt = MiPageTableToVirtual( &Pdt[ PdtIndex ] );
		Pdt[ PdtIndex ] |= Flags;

		Pt[ PtIndex ] = Flags;
	}

}