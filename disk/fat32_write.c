
#include "fat32.h"

NTSTATUS
FsFat32WriteDirectoryFile(
	__in PPARTITION_OBJECT Partition,
	__in PFILE_OBJECT FileObject,
	__in PIO_STATUS_BLOCK UserIosb
	)
{
	Partition;
	FileObject;
	UserIosb;

	return STATUS_SUCCESS;
}
