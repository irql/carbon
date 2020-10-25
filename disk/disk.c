


#include "driver.h"

PDISK_OBJECT
FsGetDiskById(
	__in ULONG Id
	)
{
	PLIST_ENTRY Flink = &DiskHead->DiskLinks;

	do {
		PDISK_OBJECT Partition = CONTAINING_RECORD(Flink, DISK_OBJECT, DiskLinks.Flink);

		if (Partition->Identifier == Id) {

			return Partition;
		}

		Flink = Flink->Flink;
	} while (Flink != &DiskHead->DiskLinks);

	return NULL;
}

PPARTITION_OBJECT
FsGetPartitionById(
	__in PDISK_OBJECT Disk,
	__in ULONG Id
	)
{

	PLIST_ENTRY Flink = Disk->PartitionLinks;

	do {
		PPARTITION_OBJECT Partition = CONTAINING_RECORD(Flink, PARTITION_OBJECT, PartitionLinks.Flink);

		if (Partition->Identifier == Id) {

			return Partition;
		}

		Flink = Flink->Flink;
	} while (Flink != Disk->PartitionLinks);

	return NULL;
}

