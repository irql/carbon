

#include "driver.h"

PDISK_OBJECT DiskHead = NULL;

PDISK_OBJECT
FsDiskCreateObject(

	)
{
	PDISK_OBJECT NewDisk = (PDISK_OBJECT)ExAllocatePoolWithTag(sizeof(DISK_OBJECT), TAGEX_DISK);

	if (DiskHead == NULL) {

		DiskHead = NewDisk;
		KeInitializeListHead(&DiskHead->DiskLinks);

	}
	else {

		KeInsertListEntry(&DiskHead->DiskLinks, &NewDisk->DiskLinks);
	}

	return NewDisk;
}
