/*++

Module ObjectName:

	pool.c

Abstract:

	Pool executive.

--*/

#pragma once


#define POOL_BLOCKS_PER_ALLOCATION_BLOCK_TABLE	0xff
#define POOL_BLOCKS_PER_ALLOCATED_BLOCK_TABLE	0xff
#define POOL_BLOCK_SIZE							0x40
#define POOL_BLOCK_UNUSED						0
#define POOL_BLOCK_INVALID						((unsigned)-1)

typedef struct _POOL_HEADER {
	PLIST_ENTRY				AllocationBlockTableLinks;
	PLIST_ENTRY				AllocatedBlockTableLinks;
} POOL_HEADER, *PPOOL_HEADER;

typedef struct _POOL_ALLOCATED_BLOCK {
	PVOID					PoolPointer;
	ULONG					PoolSize;
	ULONG					PoolTag;
} POOL_ALLOCATED_BLOCK, *PPOOL_ALLOCATED_BLOCK;

typedef struct _POOL_ALLOCATED_BLOCK_TABLE {
	LIST_ENTRY				AllocatedBlockTableLinks;
	POOL_ALLOCATED_BLOCK	AllocatedBlocks[ 1 ];
} POOL_ALLOCATED_BLOCK_TABLE, *PPOOL_ALLOCATED_BLOCK_TABLE;

typedef struct _POOL_ALLOCATION_BLOCK {
	ULONG64					Block;
	ULONG64					Bitmap;
} POOL_ALLOCATION_BLOCK, *PPOOL_ALLOCATION_BLOCK;

typedef struct _POOL_ALLOCATION_BLOCK_TABLE {
	LIST_ENTRY				AllocationBlockTableLinks;
	POOL_ALLOCATION_BLOCK	AllocationBlocks[ 1 ];
} POOL_ALLOCATION_BLOCK_TABLE, *PPOOL_ALLOCATION_BLOCK_TABLE;

PPOOL_ALLOCATION_BLOCK_TABLE
ExpInitAllocationBlockTable(

);

PPOOL_ALLOCATED_BLOCK_TABLE
ExpInitAllocatedBlockTable(

);

VOID
ExpInitializePoolAllocation(

);

ULONG
ExpFindAllocatedBlock(
	__in PPOOL_ALLOCATED_BLOCK_TABLE AllocatedBlockTable
);

ULONG
ExpFindAllocationBlock(
	__in ULONG NumberOfBytes,
	__in PPOOL_ALLOCATION_BLOCK_TABLE AllocationBlockTable
);

PVOID
ExAllocatePoolWithTag(
	__in ULONG NumberOfBytes,
	__in ULONG PoolTag
);

VOID
ExFreePoolWithTag(
	__in PVOID Pool,
	__in ULONG PoolTag
);