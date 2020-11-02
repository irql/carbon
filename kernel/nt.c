


#include <carbsup.h>
#include "nt.h"

NTSTATUS
NtCreateFile(
	__out PHANDLE            FileHandle,
	__out PIO_STATUS_BLOCK   IoStatusBlock,
	__in  ACCESS_MASK        DesiredAccess,
	__in  ULONG              Disposition,
	__in  POBJECT_ATTRIBUTES ObjectAttributes
)
{

}