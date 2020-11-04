


#include <carbsup.h>
#include "mi.h"
#include "ldrpsup.h"
#include "ki.h"

EXTERN PLIST_ENTRY g_PhysicalRegionHead;
EXTERN KLOCKED_LIST g_AddressSpaceHead;

EXTERN ADDRESS_SPACE_DESCRIPTOR g_KernelPageTable;

EXTERN PLIST_ENTRY g_PageTableAllocationHead;

#define NEW_ADDRESS( x ) ( ( ULONG64 )( x ) ) += ( 0xFFFF'8000'0000'0000 - 0x20'0000 )
#define REGION_LENGTH_TO_BITMAP_SIZE( x ) ( (( ( x ) + ( 0x1000 * 0x40 - 1) ) / ( 0x1000 * 0x40 )) * 8 )

ULONG64
MiBspBootMapIdentity(
	__in ULONG64 Address,
	__in ULONG64 ByteCount
)
{
	STATIC ULONG64 NextPageTableBase = 0x17000;

	ULONG64 PageCount = ROUND_TO_PAGES( ByteCount ) / 0x1000;

	USHORT Pml4tIndex, PdptIndex, PdtIndex, PtIndex;

	for ( ULONG64 i = 0; i < PageCount; i++ ) {
		MiVirtualToIndex( Address + i * 0x1000, &Pml4tIndex, &PdptIndex, &PdtIndex, &PtIndex );

		ULONG64* Pml4t = ( ULONG64* )__readcr3( );

		if ( !( Pml4t[ Pml4tIndex ] & EntryPresent ) ) {

			Pml4t[ Pml4tIndex ] = NextPageTableBase | EntryPresent | EntryWriteable;
			NextPageTableBase += 0x1000;
		}

		ULONG64* Pdpt = ( ULONG64* )( Pml4t[ Pml4tIndex ] & ~0xfff );

		if ( !( Pdpt[ PdptIndex ] & EntryPresent ) ) {

			Pdpt[ PdptIndex ] = NextPageTableBase | EntryPresent | EntryWriteable;
			NextPageTableBase += 0x1000;
		}

		ULONG64* Pdt = ( ULONG64* )( Pdpt[ PdptIndex ] & ~0xfff );

		if ( !( Pdt[ PdtIndex ] & EntryPresent ) ) {

			Pdt[ PdtIndex ] = NextPageTableBase | EntryPresent | EntryWriteable;
			NextPageTableBase += 0x1000;
		}
		ULONG64* Pt = ( ULONG64* )( Pdt[ PdtIndex ] & ~0xfff );

		Pt[ PtIndex ] = ( Address + i * 0x1000 ) | EntryPresent | EntryWriteable;
	}

	return Address;
}

ULONG64
MiInitRegionDescriptor(
	__in PE820MM_ENTRY Entry,
	__in PPHYSICAL_REGION_DESCRIPTOR RegionDescriptor
)
{
	if ( g_PhysicalRegionHead == NULL ) {

		KeInitializeListHead( &RegionDescriptor->RegionLinks );
		g_PhysicalRegionHead = &RegionDescriptor->RegionLinks;
	}
	else {

		KeInsertListEntry( g_PhysicalRegionHead, &RegionDescriptor->RegionLinks );
	}

	_memset( &RegionDescriptor->Bitmap[ 0 ], 0, REGION_LENGTH_TO_BITMAP_SIZE( Entry->RegionLength ) );

	RegionDescriptor->RegionBase = Entry->RegionBase;
	RegionDescriptor->RegionLength = Entry->RegionLength;
	RegionDescriptor->RegionLock.ThreadLocked = 0;

	return REGION_LENGTH_TO_BITMAP_SIZE( Entry->RegionLength ) + sizeof( PHYSICAL_REGION_DESCRIPTOR );
}

#pragma optimize("", off)

ULONG64
MiInitMemoryManager(
	__in PE820MM MemoryMap
)
{

	ULONG64 RegionDescriptorsSize = 0;
	ULONG64 TotalMemorySize = 0;
	ULONG64 RegionDescriptorsPhysicalBase = 0;

	for ( ULONG32 i = 0; i < MemoryMap->EntryCount; i++ ) {

		if ( MemoryMap->Entries[ i ].RegionType == E820_USABLE ) {

			MemoryMap->Entries[ i ].RegionBase &= ~0xFFF;
			MemoryMap->Entries[ i ].RegionLength &= ~0xFFF;

			if ( MemoryMap->Entries[ i ].RegionType == E820_USABLE &&
				MemoryMap->Entries[ i ].RegionLength >= 0x1000 ) {

				RegionDescriptorsSize += REGION_LENGTH_TO_BITMAP_SIZE( MemoryMap->Entries[ i ].RegionLength ) + sizeof( PHYSICAL_REGION_DESCRIPTOR );
				TotalMemorySize += MemoryMap->Entries[ i ].RegionLength;
			}
		}
	}

	RegionDescriptorsSize = ROUND_TO_PAGES( RegionDescriptorsSize );

	LDR_INFO_BLOCK KernelMap;
	LdrpSupGetInfoBlock( ( PVOID )KERNEL_IMAGE_BASE, &KernelMap );

	/*
		convert addresses to physical's, by the boot loader's mapping.
	*/

	( ( ULONG64 )KernelMap.ModuleStart ) -= ( 0xFFFF'8000'0000'0000 - 0x20'0000 );
	( ( ULONG64 )KernelMap.ModuleEnd ) -= ( 0xFFFF'8000'0000'0000 - 0x20'0000 );
	( ( ULONG64 )KernelMap.ModuleEntry ) -= ( 0xFFFF'8000'0000'0000 - 0x20'0000 );

	for ( ULONG i = 0; i < MemoryMap->EntryCount; i++ ) {

		if ( MemoryMap->Entries[ i ].RegionType == E820_USABLE ) {

			ULONG64 Length = MemoryMap->Entries[ i ].RegionLength;
			ULONG64 Base = MemoryMap->Entries[ i ].RegionBase;

			while ( Length > RegionDescriptorsSize && Base < ( ULONG64 )KernelMap.ModuleEnd ) {

				Base += 0x1000;
				Length -= 0x1000;
			}

			if ( Length > RegionDescriptorsSize ) {

				RegionDescriptorsPhysicalBase = Base;
				break;
			}
			else {

				RegionDescriptorsPhysicalBase = 0;
			}
		}
	}

	if ( RegionDescriptorsPhysicalBase == 0 ) {

		KiBspBootBugcheck( 0x80000001, 0, 0, 0, 0 );
	}

	MiBspBootMapIdentity( RegionDescriptorsPhysicalBase, RegionDescriptorsSize );

	//KiBspBootBugcheck( ( ULONG32 )RegionDescriptorsPhysicalBase, ( ULONG32 )RegionDescriptorsSize, 0, 0, 0 );

	g_PhysicalRegionHead = NULL;

	for ( ULONG64 i = 0, TemporaryPhysicalBase = RegionDescriptorsPhysicalBase; i < MemoryMap->EntryCount; i++ ) {

		if ( MemoryMap->Entries[ i ].RegionType == E820_USABLE &&
			MemoryMap->Entries[ i ].RegionLength >= 0x1000 ) {

			TemporaryPhysicalBase += MiInitRegionDescriptor( &MemoryMap->Entries[ i ], ( PPHYSICAL_REGION_DESCRIPTOR )TemporaryPhysicalBase );
		}
	}

	//0000000000215010
	//000000000021503b

	g_KernelPageTable.BasePhysical = ( ULONG64 )0x10000;
	g_KernelPageTable.BaseVirtual = ( ULONG64* )0x10000;

	PPAGE_TABLE_ALLOCATION_ENTRY PageTableAlloc = ( PPAGE_TABLE_ALLOCATION_ENTRY )
		MiBspBootMapIdentity( ( ( RegionDescriptorsPhysicalBase + RegionDescriptorsSize + 0x1F'FFFF ) / 0x20'0000 ) * 0x20'0000, 0x20'0000 );

	KeInitializeListHead( &PageTableAlloc->AllocationLinks );
	g_PageTableAllocationHead = &PageTableAlloc->AllocationLinks;

	PageTableAlloc->AllocationLock.ThreadLocked = 0;
	PageTableAlloc->BasePhysical = ( ULONG64 )PageTableAlloc;
	PageTableAlloc->BaseVirtual = ( ULONG64 )PageTableAlloc;

	_memset( &PageTableAlloc->Bitmap[ 0 ], 0, sizeof( PageTableAlloc->Bitmap ) );

	ULONG64 NewPml4tPhysical = 0;
	ULONG64 *NewPml4tVirtual = MiPageTableToVirtual( &NewPml4tPhysical );
	NewPml4tPhysical &= ~0xfff;

	ULONG64 *NewPdptVirtualLo = MiPageTableToVirtual( &NewPml4tVirtual[ 0 ] );
	ULONG64 *NewPdptVirtualHi = MiPageTableToVirtual( &NewPml4tVirtual[ 256 ] );

	NewPml4tVirtual[ 0 ] |= EntryPresent | EntryWriteable;
	NewPml4tVirtual[ 256 ] |= EntryPresent | EntryWriteable;

	ULONG64 *NewPdtVirtualLo = MiPageTableToVirtual( &NewPdptVirtualLo[ 0 ] );
	ULONG64 *NewPdtVirtualHi = MiPageTableToVirtual( &NewPdptVirtualHi[ 0 ] );

	NewPdptVirtualLo[ 0 ] |= EntryPresent | EntryWriteable;
	NewPdptVirtualHi[ 0 ] |= EntryPresent | EntryWriteable;

	ULONG64 *NewPtVirtualLo = MiPageTableToVirtual( &NewPdtVirtualLo[ 0 ] );
	ULONG64 *NewPtVirtualHi = MiPageTableToVirtual( &NewPdtVirtualHi[ 0 ] );

	NewPdtVirtualLo[ 0 ] |= EntryPresent | EntryWriteable;
	NewPdtVirtualHi[ 0 ] |= EntryPresent | EntryWriteable;

	for ( ULONG32 i = 0; i < 512; i++ ) {

		NewPtVirtualLo[ i ] = ( i * 0x1000 ) | EntryPresent | EntryWriteable;
	}

	MiMarkPhysical( 0, 512, TRUE );

	for ( ULONG32 i = 0; i < 512; i++ ) {

		NewPtVirtualHi[ i ] = ( 0x20'0000 + i * 0x1000 ) | EntryPresent | EntryWriteable | EntryCpuGlobal;
	}

	MiMarkPhysical( 0x20'0000, 512, TRUE );

	/*
		map @PageTableAlloc
	*/

	ULONG64 Index = PageTableAlloc->BasePhysical - 0x20'0000;

	Index /= 0x20'0000;

	ULONG64 *NewPtVirtual1 = MiPageTableToVirtual( &NewPdtVirtualHi[ Index ] );
	NewPdtVirtualHi[ Index ] |= EntryPresent | EntryWriteable;

	for ( ULONG32 i = 0; i < 512; i++ ) {

		NewPtVirtual1[ i ] = ( PageTableAlloc->BasePhysical + i * 0x1000 ) | EntryPresent | EntryWriteable;
	}

	MiMarkPhysical( PageTableAlloc->BasePhysical, 512, TRUE );

	//__writecr2( ( unsigned long long )PageTableAlloc->BasePhysical );
	//__halt( );


	//PageTableAlloc->BaseVirtual -= 0x20'0000;
	//PageTableAlloc->BaseVirtual += 0xFFFF'8000'0000'0000;
	NEW_ADDRESS( PageTableAlloc->BaseVirtual );

	g_KernelPageTable.BasePhysical = NewPml4tPhysical;
	g_KernelPageTable.BaseVirtual = ( ULONG64* )( ( ( ULONG64 )NewPml4tVirtual ) + ( 0xFFFF'8000'0000'0000 - 0x20'0000 ) );
	g_KernelPageTable.Process = NULL;
	g_KernelPageTable.Flags = 0;

	g_AddressSpaceHead.List = &g_KernelPageTable.TableLinks;
	KeInitializeListHead( g_AddressSpaceHead.List );

	g_KernelPageTable.TableLock.ThreadLocked = 0;

	MiSetAddressSpace( &g_KernelPageTable );

	NEW_ADDRESS( g_PageTableAllocationHead );
	NEW_ADDRESS( g_PageTableAllocationHead->Flink );
	NEW_ADDRESS( g_PageTableAllocationHead->Blink );

	NEW_ADDRESS( g_PhysicalRegionHead );

	PLIST_ENTRY Flink = g_PhysicalRegionHead;
	do {

		NEW_ADDRESS( Flink->Flink );
		NEW_ADDRESS( Flink->Blink );

		Flink = Flink->Flink;
	} while ( Flink != g_PhysicalRegionHead );

	MiMarkPhysical( RegionDescriptorsPhysicalBase, RegionDescriptorsSize, TRUE );

	//
	//	map all 256 pml4t entries, this is so that if kernel memory is allocated 
	//	all other page tables are updated (the top 256 entries are copied from
	//	page table to page table), uses 2mb of memory.
	//

	for ( ULONG32 i = 0; i < 256; i++ ) {

		MiPageTableToVirtual( &g_KernelPageTable.BaseVirtual[ 256 + i ] );
	}

	return TotalMemorySize;
}

#pragma optimize("", on)
