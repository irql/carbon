

#include "fat32.h"

ULONG32
FspFat32QueryFatTable(
	__in PFAT32FS Drive,
	__in ULONG32 Cluster
	)
{

	ULONG32 FatSectorOffset = (Cluster * 4) / Drive->Bpb->Dos2_00Bpb.BytesPerSector;
	ULONG32 FatSectorIndex = ((Cluster * 4) % Drive->Bpb->Dos2_00Bpb.BytesPerSector) / 4;
	ULONG32* FatTable = ExAllocatePoolWithTag(Drive->Bpb->Dos2_00Bpb.BytesPerSector, TAGEX_FAT);

	if (!NT_SUCCESS(Drive->Drive->Read(Drive->Access, Drive->Bpb->Dos2_00Bpb.ReservedSectors + FatSectorOffset, (PVOID)FatTable, Drive->Bpb->Dos2_00Bpb.BytesPerSector))) {

		return FAT32_END_OF_CHAIN;
	}

	return FatTable[FatSectorIndex] & 0x0FFFFFFF;
}
