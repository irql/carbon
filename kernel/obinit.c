/*++

Module ObjectName:

	obquery.c

Abstract:

	Object querying.

--*/

#include <carbsup.h>
#include "ob.h"
#include "ldrsup.h"
#include "ke.h"
#include "io.h"

POBJECT_TYPE_DESCRIPTOR ObjectTypeThread = 0;
POBJECT_TYPE_DESCRIPTOR ObjectTypeDevice = 0;
POBJECT_TYPE_DESCRIPTOR ObjectTypeModule = 0;
POBJECT_TYPE_DESCRIPTOR ObjectTypeProcess = 0;
POBJECT_TYPE_DESCRIPTOR ObjectTypeSymbolicLink = 0;
POBJECT_TYPE_DESCRIPTOR ObjectTypeDriver = 0;
POBJECT_TYPE_DESCRIPTOR ObjectTypeIoCommunicationBlock = 0;

NTSTATUS
ObpInitializeObjectType(
	__in PWCHAR ObjectName,
	__in ULONG32 ObjectLength,
	__in ULONG32 AllocationTag,
	__out POBJECT_TYPE_DESCRIPTOR* ObjectType
)
{

	*ObjectType = ExAllocatePoolWithTag( sizeof( OBJECT_TYPE_DESCRIPTOR ), TAGEX_OB );

	if ( *ObjectType == NULL ) {

		return STATUS_UNSUCCESSFUL;
	}

	_memset( *ObjectType, 0, sizeof( OBJECT_TYPE_DESCRIPTOR ) );

	RtlInitUnicodeString( &( *ObjectType )->ObjectName, ObjectName );

	( *ObjectType )->ObjectLength = ObjectLength;
	( *ObjectType )->PoolTag = AllocationTag;
	( *ObjectType )->PoolUsage = 0;
	( *ObjectType )->TypeList.Flink = NULL;
	( *ObjectType )->TypeList.Blink = NULL;
	( *ObjectType )->ObjectList.List = NULL;
	( *ObjectType )->ObjectList.Lock.ThreadLocked = 0;

	( *ObjectType )->TotalNumberOfHandles = 0;
	( *ObjectType )->TotalNumberOfObjects = 0;

	return STATUS_SUCCESS;
}

NTSTATUS
ObInitializeObjectType(
	__in PWCHAR ObjectName,
	__in ULONG32 ObjectLength,
	__in ULONG32 AllocationTag,
	__out POBJECT_TYPE_DESCRIPTOR* ObjectType
)
{
	NTSTATUS ntStatus = ObpInitializeObjectType( ObjectName, ObjectLength, AllocationTag, ObjectType );

	if ( NT_SUCCESS( ntStatus ) )
		KeInsertListEntry( &ObjectTypeThread->TypeList, &( *ObjectType )->TypeList );

	return ntStatus;
}

VOID
ObpInitializeObjectManager(

)
{

	ObpInitializeObjectType( L"ThreadObject", sizeof( KTHREAD ), 'psiD', &ObjectTypeThread );
	KeInitializeListHead( &ObjectTypeThread->TypeList );

	ObInitializeObjectType( L"DeviceObject", sizeof( DEVICE_OBJECT ), ' veD', &ObjectTypeDevice );
	ObInitializeObjectType( L"ModuleObject", sizeof( KMODULE ), 'udoM', &ObjectTypeModule );
	ObInitializeObjectType( L"ProcessObject", sizeof( KPROCESS ), 'corP', &ObjectTypeProcess );
	ObInitializeObjectType( L"SymbolicLinkObject", sizeof( OBJECT_SYMBOLIC_LINK ), 'kniL', &ObjectTypeSymbolicLink );
	ObInitializeObjectType( L"DriverObject", sizeof( DRIVER_OBJECT ), ' vrD', &ObjectTypeDriver );
	ObInitializeObjectType( L"IoCommunicationObject", sizeof( IOCB ), 'bcoI', &ObjectTypeIoCommunicationBlock );
}
