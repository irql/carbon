

#include <carbsup.h>
#include "obp.h"
#include "ki.h"

NTSTATUS
ObDestroyObject(
	__in PVOID Object
)
{

	POBJECT_ENTRY_HEADER ObjectHeader = OB_OBJ2HEADER( Object );
	ObjectHeader;
	//KeRemoveListEntry( &ObjectHeader->ObjectList );
	//ObjectHeader->TypeDescriptor->PoolUsage -= ( ObjectHeader->TypeDescriptor->ObjectLength + sizeof( OBJECT_ENTRY_HEADER ) );
	//ExFreePoolWithTag( ObjectHeader, ObjectHeader->TypeDescriptor->PoolTag );

	return STATUS_SUCCESS;
}

