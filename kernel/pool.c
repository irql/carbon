/*++

Module ObjectName:

	pool.c

Abstract:

	Pool executive.

--*/

#include <carbsup.h>
#include "exp.h"


POOL_HEADER PoolHeader;

PPOOL_ALLOCATION_BLOCK_TABLE
ExpInitAllocationBlockTable(

) {
	PPOOL_ALLOCATION_BLOCK_TABLE AllocationBlocks = ( PPOOL_ALLOCATION_BLOCK_TABLE )MmAllocateMemory( 0x1000, PAGE_READ | PAGE_WRITE | PAGE_EXECUTE );

	_memset( AllocationBlocks, POOL_BLOCK_UNUSED, 0x1000 );

	AllocationBlocks->AllocationBlockTableLinks.Flink = NULL;
	AllocationBlocks->AllocationBlockTableLinks.Blink = NULL;

	return AllocationBlocks;
}

PPOOL_ALLOCATED_BLOCK_TABLE
ExpInitAllocatedBlockTable(

) {
	PPOOL_ALLOCATED_BLOCK_TABLE AllocatedBlocks = ( PPOOL_ALLOCATED_BLOCK_TABLE )MmAllocateMemory( 0x1000, PAGE_READ | PAGE_WRITE );

	_memset( AllocatedBlocks, POOL_BLOCK_UNUSED, 0x1000 );

	AllocatedBlocks->AllocatedBlockTableLinks.Flink = NULL;
	AllocatedBlocks->AllocatedBlockTableLinks.Blink = NULL;

	return AllocatedBlocks;
}

VOID
ExpInitializePoolAllocation(

) {

	PPOOL_ALLOCATION_BLOCK_TABLE AllocationBlockPage = ExpInitAllocationBlockTable( );
	KeInitializeListHead( &AllocationBlockPage->AllocationBlockTableLinks );
	PoolHeader.AllocationBlockTableLinks = &AllocationBlockPage->AllocationBlockTableLinks;

	PPOOL_ALLOCATED_BLOCK_TABLE AllocatedBlockTable = ExpInitAllocatedBlockTable( );
	KeInitializeListHead( &AllocatedBlockTable->AllocatedBlockTableLinks );
	PoolHeader.AllocatedBlockTableLinks = &AllocatedBlockTable->AllocatedBlockTableLinks;


}

ULONG
ExpFindAllocatedBlock(
	__in PPOOL_ALLOCATED_BLOCK_TABLE AllocatedBlockTable
) {

	for ( ULONG i = 0; i < POOL_BLOCKS_PER_ALLOCATED_BLOCK_TABLE; i++ ) {

		if ( AllocatedBlockTable->AllocatedBlocks[ i ].PoolSize == 0 ) {

			return i;
		}
	}
	return POOL_BLOCK_INVALID;
}

ULONG
ExpFindAllocationBlock(
	__in ULONG NumberOfBytes,
	__in PPOOL_ALLOCATION_BLOCK_TABLE AllocationBlockTable
)
{

	ULONG NumberOfBitBlocks = ( NumberOfBytes + ( POOL_BLOCK_SIZE - 1 ) ) / POOL_BLOCK_SIZE;
	ULONG NumberOfBlocks = ( NumberOfBitBlocks * POOL_BLOCK_SIZE ) / 0x1000;

	ULONG NumberOfBitBlocksFound = NumberOfBitBlocks;
	ULONG NumberOfBlocksFound = NumberOfBlocks;
	ULONG FoundBitBlockStartIndex = POOL_BLOCK_INVALID;

	//use mmallocatememory. (255 pages, 0xff000)
	if ( NumberOfBlocks > ( POOL_BLOCKS_PER_ALLOCATION_BLOCK_TABLE * 0x1000 ) )
		return POOL_BLOCK_INVALID;

	for ( ULONG i = 0; i < ( POOL_BLOCKS_PER_ALLOCATION_BLOCK_TABLE * 64 ); ) {

		if ( AllocationBlockTable->AllocationBlocks[ i / 64 ].Bitmap == -1i64 ) {
			NumberOfBitBlocksFound = NumberOfBitBlocks;
			NumberOfBlocksFound = NumberOfBlocks;
			FoundBitBlockStartIndex = POOL_BLOCK_INVALID;
			i += 64;
			continue;
		}

		if ( AllocationBlockTable->AllocationBlocks[ i / 64 ].Block == POOL_BLOCK_UNUSED ) {

			if ( FoundBitBlockStartIndex == POOL_BLOCK_INVALID )
				FoundBitBlockStartIndex = i;
			else if ( i != 0 && AllocationBlockTable->AllocationBlocks[ ( i / 64 ) - 1 ].Block != POOL_BLOCK_UNUSED ) {
				NumberOfBitBlocksFound = NumberOfBitBlocks;
				NumberOfBlocksFound = NumberOfBlocks;
				FoundBitBlockStartIndex = i;
			}

			if ( NumberOfBitBlocksFound <= 64 )
				return FoundBitBlockStartIndex;
			else
				NumberOfBitBlocksFound -= 64;

			if ( NumberOfBlocksFound >= 1 )
				NumberOfBlocksFound--;

			i += 64;
			continue;
		}
		else if ( NumberOfBlocksFound == 0 ) {//we cant give them a bunch of addresses, it must be linear.

			if ( ( AllocationBlockTable->AllocationBlocks[ i / 64 ].Bitmap & ( 1i64 << ( i % 64 ) ) ) == 0 ) {

				if ( FoundBitBlockStartIndex == POOL_BLOCK_INVALID )
					FoundBitBlockStartIndex = i;

				NumberOfBitBlocksFound--;

				if ( NumberOfBitBlocksFound == 0 )
					return FoundBitBlockStartIndex;
			}
			else {
				NumberOfBitBlocksFound = NumberOfBitBlocks;
				NumberOfBlocksFound = NumberOfBlocks;
				FoundBitBlockStartIndex = POOL_BLOCK_INVALID;
			}
		}
		i++;
	}

	if ( NumberOfBlocksFound == 0 &&
		NumberOfBitBlocksFound == 0 )
		return FoundBitBlockStartIndex;

	return POOL_BLOCK_INVALID;
}

PVOID
ExAllocatePoolWithTag(
	__in ULONG NumberOfBytes,
	__in ULONG PoolTag
) {

	PLIST_ENTRY Flink = PoolHeader.AllocationBlockTableLinks;
	PPOOL_ALLOCATION_BLOCK_TABLE AllocationTable = NULL;
	PPOOL_ALLOCATED_BLOCK_TABLE AllocatedTable = NULL;

	ULONG AllocationTableBitIndex = POOL_BLOCK_INVALID;
	ULONG AllocatedTableIndex = POOL_BLOCK_INVALID;

	ULONG NumberOfBitBlocks = ( NumberOfBytes + ( POOL_BLOCK_SIZE - 1 ) ) / POOL_BLOCK_SIZE;
	ULONG NumberOfBlocks = ( NumberOfBitBlocks * POOL_BLOCK_SIZE ) / 0x1000;

	BOOLEAN SecondTry = 0;

	if ( NumberOfBytes == 0 )
		return NULL;

SecondTry1:
	do {
		AllocationTable = CONTAINING_RECORD( Flink, POOL_ALLOCATION_BLOCK_TABLE, AllocationBlockTableLinks );
		AllocationTableBitIndex = ExpFindAllocationBlock( NumberOfBytes, AllocationTable );

		if ( AllocationTableBitIndex != POOL_BLOCK_INVALID )
			break;

		Flink = Flink->Flink;
	} while ( Flink != PoolHeader.AllocationBlockTableLinks );

	if ( AllocationTableBitIndex == POOL_BLOCK_INVALID ) {
		if ( SecondTry == 0 ) {
			KeInsertListEntry( PoolHeader.AllocationBlockTableLinks, &ExpInitAllocationBlockTable( )->AllocationBlockTableLinks );
			SecondTry = 1;
			goto SecondTry1;
		}
		else {
			return NULL;
		}
	}
	SecondTry = 0;

SecondTry2:
	Flink = PoolHeader.AllocatedBlockTableLinks;
	do {
		AllocatedTable = CONTAINING_RECORD( Flink, POOL_ALLOCATED_BLOCK_TABLE, AllocatedBlockTableLinks );
		AllocatedTableIndex = ExpFindAllocatedBlock( AllocatedTable );

		if ( AllocatedTableIndex != POOL_BLOCK_INVALID )
			break;

		Flink = Flink->Flink;
	} while ( Flink != PoolHeader.AllocatedBlockTableLinks );

	if ( AllocatedTableIndex == POOL_BLOCK_INVALID ) {
		if ( SecondTry == 0 ) {
			KeInsertListEntry( PoolHeader.AllocatedBlockTableLinks, &ExpInitAllocatedBlockTable( )->AllocatedBlockTableLinks );
			SecondTry = 1;
			goto SecondTry2;
		}
		else {
			return NULL;
		}
	}
	SecondTry = 0;

	ULONG64 Blocks = 0;
	if ( NumberOfBlocks != 0 ) {

		Blocks = ( ULONG64 )MmAllocateMemory( ( ( ULONG64 )NumberOfBlocks + ( ( ( ULONG64 )NumberOfBitBlocks % 64 ) != 0 ) ) * 0x1000, PAGE_READ | PAGE_WRITE | PAGE_EXECUTE );
	}

	for ( ULONG i = AllocationTableBitIndex; ( i - AllocationTableBitIndex ) < NumberOfBitBlocks; ) {

		if ( i % 64 == 0 ) {
			if ( AllocationTable->AllocationBlocks[ i / 64 ].Block == POOL_BLOCK_UNUSED ) {
				AllocationTable->AllocationBlocks[ i / 64 ].Bitmap = 0i64;

				if ( Blocks != 0 ) {

					AllocationTable->AllocationBlocks[ i / 64 ].Block = Blocks;
					Blocks += 0x1000;
				}
				else {

					AllocationTable->AllocationBlocks[ i / 64 ].Block = ( ULONG64 )MmAllocateMemory( 0x1000, PAGE_READ | PAGE_WRITE );
				}
			}

			if ( ( NumberOfBitBlocks - ( i - AllocationTableBitIndex ) ) > 64 ) {
				AllocationTable->AllocationBlocks[ i / 64 ].Bitmap = ( ULONG64 )-1i64;
				i += 64;
				continue;
			}
		}

		AllocationTable->AllocationBlocks[ i / 64 ].Bitmap |= ( 1i64 << ( i % 64 ) );
		i++;
	}

	AllocatedTable->AllocatedBlocks[ AllocatedTableIndex ].PoolPointer = ( PVOID )( AllocationTable->AllocationBlocks[ AllocationTableBitIndex / 64 ].Block +
		( ( ( ULONG64 )AllocationTableBitIndex % 64 ) * ( ULONG64 )POOL_BLOCK_SIZE ) );
	AllocatedTable->AllocatedBlocks[ AllocatedTableIndex ].PoolSize = NumberOfBitBlocks * ( ULONG )POOL_BLOCK_SIZE;
	AllocatedTable->AllocatedBlocks[ AllocatedTableIndex ].PoolTag = PoolTag;

	return ( PVOID )( AllocationTable->AllocationBlocks[ AllocationTableBitIndex / 64 ].Block + ( ( ( ULONG64 )AllocationTableBitIndex % 64 ) * ( ULONG64 )POOL_BLOCK_SIZE ) );
}

VOID
ExFreePoolWithTag(
	__in PVOID Pool,
	__in ULONG PoolTag
) {
	PPOOL_ALLOCATED_BLOCK AllocatedBlock = NULL;

	PLIST_ENTRY Flink = PoolHeader.AllocatedBlockTableLinks;
	do {
		PPOOL_ALLOCATED_BLOCK_TABLE AllocatedTable = CONTAINING_RECORD( Flink, POOL_ALLOCATED_BLOCK_TABLE, AllocatedBlockTableLinks );

		for ( ULONG i = 0; i < POOL_BLOCKS_PER_ALLOCATED_BLOCK_TABLE; i++ ) {
			if ( AllocatedTable->AllocatedBlocks[ i ].PoolPointer == Pool &&
				AllocatedTable->AllocatedBlocks[ i ].PoolTag == PoolTag ) {
				AllocatedBlock = &AllocatedTable->AllocatedBlocks[ i ];
			}
		}

		Flink = Flink->Flink;
	} while ( Flink != PoolHeader.AllocatedBlockTableLinks );

	if ( AllocatedBlock == NULL ) {

		//hmm should we bugcheck?
		return;
	}

	Flink = PoolHeader.AllocationBlockTableLinks;
	do {
		PPOOL_ALLOCATION_BLOCK_TABLE AllocationTable = CONTAINING_RECORD( Flink, POOL_ALLOCATION_BLOCK_TABLE, AllocationBlockTableLinks );

		for ( ULONG i = 0; i < POOL_BLOCKS_PER_ALLOCATION_BLOCK_TABLE; i++ ) {

			if ( AllocationTable->AllocationBlocks[ i ].Bitmap != 0 ) {

				if ( AllocationTable->AllocationBlocks[ i ].Block == ( ( ULONG64 )Pool & ~0xfffi64 ) ) {
					ULONG Bit = ( ( ULONG64 )Pool & 0xfff ) == 0 ? 0 : ( ( ULONG64 )Pool & 0xfff ) / POOL_BLOCK_SIZE;
					ULONG TotalBits = ( AllocatedBlock->PoolSize / POOL_BLOCK_SIZE );

					for ( ULONG j = Bit; ( j - Bit ) < TotalBits; ) {

						if ( j % 64 == 0 ) {
							if ( ( TotalBits - ( j - Bit ) ) > 64 ) {
								AllocationTable->AllocationBlocks[ i + j / 64 ].Bitmap = 0i64;

								MmFreeMemory( AllocationTable->AllocationBlocks[ i + j / 64 ].Block, 0x1000 );
								AllocationTable->AllocationBlocks[ i + j / 64 ].Block = POOL_BLOCK_UNUSED;

								j += 64;
								continue;
							}
						}

						AllocationTable->AllocationBlocks[ i + j / 64 ].Bitmap &= ~( 1i64 << ( j % 64 ) );

						if ( AllocationTable->AllocationBlocks[ i + j / 64 ].Bitmap == 0 ) {
							MmFreeMemory( AllocationTable->AllocationBlocks[ i + j / 64 ].Block, 0x1000 );
							AllocationTable->AllocationBlocks[ i + j / 64 ].Block = POOL_BLOCK_UNUSED;
						}

						j++;
					}
				}
			}
		}

	} while ( Flink != PoolHeader.AllocationBlockTableLinks );

	//DbgPrint("freed %p at %p\n", AllocatedBlock->PoolSize, AllocatedBlock->PoolPointer);
	AllocatedBlock->PoolPointer = POOL_BLOCK_UNUSED;
	AllocatedBlock->PoolSize = POOL_BLOCK_UNUSED;
	AllocatedBlock->PoolTag = POOL_BLOCK_UNUSED;
}
