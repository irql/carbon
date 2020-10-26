/*++

Module ObjectName:

	file.c

Abstract:

	I/O manager file objects.

--*/


#include <carbsup.h>
#include "ob.h"
#include "io.h"
#include "ke.h"

PFILE_OBJECT
IoAllocateFileObject(

	)
{
	
	return ExAllocatePoolWithTag(sizeof(FILE_OBJECT), TAGEX_FILE );
}

VOID
IoFreeFileObject(
	__in PFILE_OBJECT FileObject
	)
{
	
	ExFreePoolWithTag(FileObject, TAGEX_FILE);
	
	return;
}



