

#include "fat32.h"

VOID
FsFat32InitializePartition(
	__in PPARTITION_OBJECT Partition
	)
{

	Partition->FileSystem.Read = (PFS_ACCESS_ROUTINE)FsFat32ReadDirectoryFile;
	Partition->FileSystem.Write = (PFS_ACCESS_ROUTINE)FsFat32WriteDirectoryFile;

}