


#include "driver.h"

VOID
FsInitializeFileSystem(
	__in PPARTITION_OBJECT Partition
	)
{

	if (Partition->Flags & PARTITION_FLAG_FILE_SYSTEM_FAT32) {

		FsFat32InitializePartition(Partition);
	}
	else if (Partition->Flags & PARTITION_FLAG_FILE_SYSTEM_NTFS) {


	}
	else {


	}

	return;
}