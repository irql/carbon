


#include <carbsup.h>
#include "mi.h"

PLIST_ENTRY g_PhysicalRegionHead = NULL;

ULONG64
MiAllocatePhysical(
	__in ULONG64 PageCount
)
{

	PLIST_ENTRY Flink = g_PhysicalRegionHead;
	do {
		PPHYSICAL_REGION_DESCRIPTOR RegionDescriptor = CONTAINING_RECORD( Flink, PHYSICAL_REGION_DESCRIPTOR, RegionLinks );

		KeAcquireSpinLock( &RegionDescriptor->RegionLock );

		ULONG64 FoundIndex = ( ULONG64 )-1;
		ULONG64 FoundCount = 0;
		ULONG64 RemainingLength = RegionDescriptor->RegionLength;

		for ( ULONG64 i = 0; i < ( RegionDescriptor->RegionLength / 0x1000 ); i++ ) {

			if ( RemainingLength < PageCount ) {

				FoundIndex = ( ULONG64 )-1;
				FoundCount = 0;
				break;
			}

			if ( RegionDescriptor->Bitmap[ i / 64 ] == ( ULONG64 )-1 ) {

				i += 63;
				FoundIndex = ( ULONG64 )-1;
				FoundCount = 0;
				RemainingLength -= ( 64 * 0x1000 );
				continue;
			}

			if ( ( RegionDescriptor->Bitmap[ i / 64 ] & ( 1i64 << ( i % 64 ) ) ) == 0 ) {

				if ( FoundIndex == ( ULONG64 )-1 ) {

					FoundIndex = i;
				}

				FoundCount++;

				if ( FoundCount == PageCount ) {

					i = FoundIndex;
					for ( ULONG64 j = 0; j < PageCount; j++, i++ ) {

						RegionDescriptor->Bitmap[ i / 64 ] |= ( 1i64 << ( i % 64 ) );
					}

					KeReleaseSpinLock( &RegionDescriptor->RegionLock );

					return RegionDescriptor->RegionBase + FoundIndex * 0x1000;
				}
			}
			else {

				RemainingLength -= 0x1000;
				FoundIndex = ( ULONG64 )-1;
				FoundCount = 0;
			}
		}


		KeReleaseSpinLock( &RegionDescriptor->RegionLock );

		Flink = Flink->Flink;
	} while ( Flink != g_PhysicalRegionHead );

	/* panic. */

	__halt( );

	return 0;
}

VOID
MiMarkPhysical(
	__in ULONG64 Address,
	__in ULONG64 PageCount,
	__in BOOLEAN Marking
)
{
#if 1
	ULONG64 PagesLeft = PageCount;

	ULONG64 SearchAddress = Address + ( PageCount - PagesLeft ) * 0x1000;
	PLIST_ENTRY Flink = g_PhysicalRegionHead;

	do {
		PPHYSICAL_REGION_DESCRIPTOR RegionDescriptor = CONTAINING_RECORD( Flink, PHYSICAL_REGION_DESCRIPTOR, RegionLinks );

		KeAcquireSpinLock( &RegionDescriptor->RegionLock );

		for ( ULONG32 i = 0; i < PageCount; i++ ) {

			if ( SearchAddress < RegionDescriptor->RegionBase ) {

				PagesLeft--;

				if ( PagesLeft == 0 ) {
					KeReleaseSpinLock( &RegionDescriptor->RegionLock );

					return;
				}

				SearchAddress = Address + ( PageCount - PagesLeft ) * 0x1000;

				continue;
			}

			if ( SearchAddress >= RegionDescriptor->RegionBase &&
				SearchAddress < ( RegionDescriptor->RegionBase + RegionDescriptor->RegionLength ) ) {

				ULONG64 Index = ( SearchAddress - RegionDescriptor->RegionBase ) / 0x1000;

				if ( Marking ) {

					RegionDescriptor->Bitmap[ Index / 64 ] |= ( 1i64 << ( Index % 64 ) );
				}
				else {

					RegionDescriptor->Bitmap[ Index / 64 ] &= ~( 1i64 << ( Index % 64 ) );
				}

				PagesLeft--;

				if ( PagesLeft == 0 ) {
					KeReleaseSpinLock( &RegionDescriptor->RegionLock );

					return;
				}

				SearchAddress = Address + ( PageCount - PagesLeft ) * 0x1000;
			}
			else {

				break;
			}
		}

		KeReleaseSpinLock( &RegionDescriptor->RegionLock );

		Flink = Flink->Flink;
	} while ( Flink != g_PhysicalRegionHead );
	//breaks because if the FUcking address is not in range it quits.

#endif
	return;
}

