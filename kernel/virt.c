


#include <carbsup.h>
#include "mi.h"

EXTERN ADDRESS_SPACE_DESCRIPTOR g_KernelPageTable;

ULONG64
MiAllocateVirtualAt(
	__in ULONG64 Virtual,
	__in ULONG64 PageCount,
	__in ULONG64 Flags
)
{
	PADDRESS_SPACE_DESCRIPTOR AddressSpace = MiGetAddressSpace( );

	KeAcquireSpinLock( &AddressSpace->TableLock );

	ULONG64 EntryFlags = MiFlagsToEntryFlags( Flags );

	USHORT Pml4tIndex, PdptIndex, PdtIndex, PtIndex;

	for ( ULONG64 i = 0; i < PageCount; i++ ) {

		MiVirtualToIndex( Virtual + ( i * 0x1000 ), &Pml4tIndex, &PdptIndex, &PdtIndex, &PtIndex );

		if ( AddressSpace->BaseVirtual[ Pml4tIndex ] & EntryPresent ) {

			ULONG64* Pdpt = MiPageTableToVirtual( &AddressSpace->BaseVirtual[ Pml4tIndex ] );
			if ( Pdpt[ PdptIndex ] & EntryPresent ) {

				ULONG64* Pdt = MiPageTableToVirtual( &Pdpt[ PdptIndex ] );
				if ( Pdt[ PdtIndex ] & EntryPresent ) {

					ULONG64* Pt = MiPageTableToVirtual( &Pdt[ PdtIndex ] );

					if ( Pt[ PtIndex ] & EntryPresent ) {

						ULONG64 NewVirtual = MiAllocateVirtual( PageCount, Flags );

						return NewVirtual;
					}
					else {

						i++;
					}
				}
				else {

					i += ( 512 );
				}
			}
			else {

				i += ( 512 * 512 );
			}
		}
		else {

			i += ( 512 * 512 * 512 );
		}
	}

	MiMarkVirtual( Virtual, PageCount, EntryFlags );

	KeReleaseSpinLock( &AddressSpace->TableLock );

	return Virtual;
}

ULONG64
MiAllocateVirtual(
	__in ULONG64 PageCount,
	__in ULONG64 Flags
)
{

	PADDRESS_SPACE_DESCRIPTOR AddressSpace;

	ULONG64 FoundCount = PageCount;
	ULONG64 FoundVirtual = 0;

	ULONG64 EntryFlags = MiFlagsToEntryFlags( Flags );

	USHORT Pml4tIndex = 0, PdptIndex = 0, PdtIndex = 0, PtIndex = 0;

	if ( Flags & PAGE_USER ) {

		AddressSpace = MiGetAddressSpace( );
	}
	else {

		MiEnterKernelSpace( &AddressSpace );
		Pml4tIndex = 256;
	}

	KeAcquireSpinLock( &AddressSpace->TableLock );

	ULONG64* Pml4t = AddressSpace->BaseVirtual;

	for ( ; Pml4tIndex < 512; Pml4tIndex++ ) {

		if ( Pml4t[ Pml4tIndex ] & EntryPresent ) {

			ULONG64* Pdpt = ( ULONG64* )MiPageTableToVirtual( &Pml4t[ Pml4tIndex ] );

			for ( PdptIndex = 0; PdptIndex < 512; PdptIndex++ ) {

				if ( Pdpt[ PdptIndex ] & EntryPresent ) {

					ULONG64* Pdt = ( ULONG64* )MiPageTableToVirtual( &Pdpt[ PdptIndex ] );

					for ( PdtIndex = 0; PdtIndex < 512; PdtIndex++ ) {

						if ( Pdt[ PdtIndex ] & EntryPresent ) {

							ULONG64* Pt = ( ULONG64* )MiPageTableToVirtual( &Pdt[ PdtIndex ] );

							for ( PtIndex = 0; PtIndex < 512; PtIndex++ ) {

								if ( Pt[ PtIndex ] & EntryPresent ) {

									FoundCount = PageCount;
									FoundVirtual = 0;
								}
								else {

									if ( FoundVirtual == 0 ) {

										FoundVirtual = MiIndexToVirtual( Pml4tIndex, PdptIndex, PdtIndex, PtIndex );

										if ( FoundVirtual < 0x10000 ) {

											FoundVirtual = 0;
											continue;
										}
									}

									FoundCount--;

									if ( FoundCount == 0 ) {

										MiMarkVirtual( FoundVirtual, PageCount, EntryFlags );

										KeReleaseSpinLock( &AddressSpace->TableLock );
										MiLeaveKernelSpace( AddressSpace );

										return FoundVirtual;
									}
								}
							}
						}
						else {

							if ( FoundVirtual == 0 ) {

								FoundVirtual = MiIndexToVirtual( Pml4tIndex, PdptIndex, PdtIndex, 0 );

								if ( FoundVirtual < 0x10000 ) {

									FoundVirtual += 0x10000;
								}
							}

							if ( FoundCount <= ( 512 ) ) {

								MiMarkVirtual( FoundVirtual, PageCount, EntryFlags );

								KeReleaseSpinLock( &AddressSpace->TableLock );
								MiLeaveKernelSpace( AddressSpace );

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

						FoundVirtual = MiIndexToVirtual( Pml4tIndex, PdptIndex, 0, 0 );

						if ( FoundVirtual < 0x10000 ) {

							FoundVirtual += 0x10000;
						}
					}

					if ( FoundCount <= ( 512 * 512 ) ) {

						MiMarkVirtual( FoundVirtual, PageCount, EntryFlags );

						KeReleaseSpinLock( &AddressSpace->TableLock );
						MiLeaveKernelSpace( AddressSpace );

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

				FoundVirtual = MiIndexToVirtual( Pml4tIndex, 0, 0, 0 );

				if ( FoundVirtual < 0x10000 ) {

					FoundVirtual += 0x10000;
				}
			}

			if ( FoundCount <= ( 512 * 512 * 512 ) ) {

				MiMarkVirtual( FoundVirtual, PageCount, EntryFlags );

				KeReleaseSpinLock( &AddressSpace->TableLock );
				MiLeaveKernelSpace( AddressSpace );

				return FoundVirtual;
			}
			else {

				FoundCount -= ( 512 * 512 * 512 );
			}
		}
	}

	KeReleaseSpinLock( &AddressSpace->TableLock );
	MiLeaveKernelSpace( AddressSpace );

	/* Bug the The.*/

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

