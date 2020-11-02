/*++

Module ObjectName:

	mm.h

Abstract:

	!

--*/

#pragma once

#define ROUND_TO_PAGES(Bytes) (((Bytes) + 0x0FFF) & (-0x1000))

#define PAGE_GLOBAL		0x10
#define PAGE_USER		0x08
#define PAGE_EXECUTE	0x04
#define PAGE_WRITE		0x02
#define PAGE_READ		0x01

typedef struct _ADDRESS_SPACE_DESCRIPTOR *PADDRESS_SPACE_DESCRIPTOR;
typedef struct _DMA_CHUNK {
	ULONG64 BaseVirtual;
	ULONG64 BasePhysical;
	ULONG64 PageCount;

} DMA_CHUNK, *PDMA_CHUNK;

NTSYSAPI PVOID MmAllocateMemory(
	__in ULONG64 ByteCount,
	__in ULONG64 Flags
);

NTSYSAPI PVOID MmAllocateMemoryAtPhysical(
	__in ULONG64 Physical,
	__in ULONG64 ByteCount,
	__in ULONG64 Flags
);

NTSYSAPI PVOID MmAllocateMemoryAtVirtual(
	__in ULONG64 Virtual,
	__in ULONG64 ByteCount,
	__in ULONG64 Flags
);

NTSYSAPI VOID MmAllocateMemoryAt(
	__in ULONG64 Virtual,
	__in ULONG64 Physical,
	__in ULONG64 ByteCount,
	__in ULONG64 Flags
);

NTSYSAPI VOID MmFreeMemory(
	__in ULONG64 Address,
	__in ULONG64 ByteCount
);

NTSYSAPI VOID MmAllocateDmaMemory(
	__inout PDMA_CHUNK DmaChunk
);

NTSYSAPI ULONG64 MmPhysicalMapping(
	__in ULONG64 Address
);

NTSYSAPI PVOID MmAllocateContiguousMemory(
	__in ULONG64 ByteCount,
	__in ULONG64 Flags
);

NTSYSAPI BOOLEAN MmIsAddressRangeValid(
	__in PVOID   VirtualAddress,
	__in ULONG64 ByteCount
);