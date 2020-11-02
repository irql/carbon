/*++

Module ObjectName:

	obcreate.c

Abstract:

	ObjectPointer creation.

--*/

#include <carbsup.h>
#include "obp.h"

HANDLE
ObpSupplyHandleIdentifier(
	__in PHANDLE_TABLE HandleTable
)

{
	HandleTable;//allows room for changes, eg find unused.

	STATIC HANDLE HandleValue = 8;

	return HandleValue += 4;
}

NTSTATUS
ObpCreateHandle(
	__in PHANDLE_TABLE HandleTable,
	__in PVOID Object,
	__out PHANDLE Handle
)

{
	POBJECT_ENTRY_HEADER ObjectHeader = OB_OBJ2HEADER( Object );

	PHANDLE_TABLE_ENTRY HandleEntry = ExAllocatePoolWithTag( sizeof( HANDLE_TABLE_ENTRY ), TAGEX_OB );

	if ( HandleEntry == NULL ) {

		return STATUS_UNSUCCESSFUL;
	}

	HandleEntry->Object = ObjectHeader;
	HandleEntry->Value = ObpSupplyHandleIdentifier( HandleTable );

	KeAcquireSpinLock( &HandleTable->HandleLinks.Lock );
	if ( HandleTable->HandleLinks.List == NULL ) {

		KeInitializeListHead( &HandleEntry->HandleLinks );
		HandleTable->HandleLinks.List = &HandleEntry->HandleLinks;
	}
	else {

		KeInsertListEntry( HandleTable->HandleLinks.List, &HandleEntry->HandleLinks );
	}
	KeReleaseSpinLock( &HandleTable->HandleLinks.Lock );

	ObReferenceObject( Object );
	HandleTable->TotalNumberOfHandles++;

	*Handle = HandleEntry->Value;
	return STATUS_SUCCESS;
}

NTSTATUS
ObpCloseHandle(
	__in PHANDLE_TABLE HandleTable,
	__in HANDLE Handle
)
{
	KeAcquireSpinLock( &HandleTable->HandleLinks.Lock );

	PLIST_ENTRY Flink = HandleTable->HandleLinks.List;
	do {
		PHANDLE_TABLE_ENTRY HandleEntry = CONTAINING_RECORD( Flink, HANDLE_TABLE_ENTRY, HandleLinks );

		if ( HandleEntry->Value == Handle ) {

			KeRemoveListEntry( &HandleEntry->HandleLinks );

			ObDereferenceObject( HandleEntry->Object );
			ExFreePoolWithTag( HandleEntry, TAGEX_OB );
			HandleTable->TotalNumberOfHandles--;

			KeReleaseSpinLock( &HandleTable->HandleLinks.Lock );

			return STATUS_SUCCESS;
		}

		Flink = Flink->Flink;
	} while ( Flink != HandleTable->HandleLinks.List );

	KeReleaseSpinLock( &HandleTable->HandleLinks.Lock );

	return STATUS_NOT_FOUND;
}

NTSTATUS
ObCreateHandle(
	__out PHANDLE Handle,
	__in PVOID Object
)
{

	return ObpCreateHandle( ObpQueryCurrentHandleTable( ), Object, Handle );
}

NTSTATUS
ObCloseHandle(
	__in HANDLE Handle
)
{

	return ObpCloseHandle( ObpQueryCurrentHandleTable( ), Handle );
}

NTSTATUS
ObpCreateObject(
	__out PVOID* ObjectPointer,
	__in POBJECT_ATTRIBUTES ObjectAttributes,
	__in POBJECT_TYPE_DESCRIPTOR ObjectType
)
{

	if ( ObjectType == ObjectTypeSymbolicLink ) {

		/*
			haha FUCK OFF. (use ObpCreateSymbolicLink)
		*/

		return STATUS_UNSUCCESSFUL;
	}

	NTSTATUS ntStatus;
	POBJECT_ENTRY_HEADER ObjectHeader;

	ObjectHeader = ExAllocatePoolWithTag(
		ObjectType->ObjectLength + sizeof( OBJECT_ENTRY_HEADER ),
		ObjectType->PoolTag );
	ObjectType->PoolUsage += ObjectType->ObjectLength + sizeof( OBJECT_ENTRY_HEADER );
	_memset( ObjectHeader, 0, sizeof( OBJECT_ENTRY_HEADER ) + ObjectType->ObjectLength );

	*ObjectPointer = OB_HEADER2OBJ( ObjectHeader );


	KeAcquireSpinLock( &ObjectType->ObjectList.Lock );
	if ( ObjectType->ObjectList.List == NULL ) {

		KeInitializeListHead( &ObjectHeader->ObjectList );
		ObjectType->ObjectList.List = &ObjectHeader->ObjectList;
	}
	else {

		KeInsertListEntry( ObjectType->ObjectList.List, &ObjectHeader->ObjectList );
	}
	KeReleaseSpinLock( &ObjectType->ObjectList.Lock );

	ObjectHeader->TypeDescriptor = ObjectType;
	ObjectHeader->Flags = ObjectAttributes->Attributes & ( OBJ_PERMANENT | 0 );

	ObjectHeader->ReferenceCount = 1;

	if ( ObjectAttributes->ObjectName != NULL ) {
		/*
			Add to ObjectTypeSymbolicLink
		*/

		ntStatus = ObpCreateSymbolicLink( ObjectAttributes->ObjectName, *ObjectPointer, NULL );

		if ( !NT_SUCCESS( ntStatus ) ) {

			ObDereferenceObject( *ObjectPointer );
			return ntStatus;
		}

	}

	ObjectType->TotalNumberOfObjects++;

	return STATUS_SUCCESS;
}

NTSTATUS
ObCreateObject(
	__out PHANDLE ObjectHandle,
	__in POBJECT_ATTRIBUTES ObjectAttributes,
	__in POBJECT_TYPE_DESCRIPTOR ObjectType
)
{
	NTSTATUS ntStatus;
	PVOID NewObject;

	ntStatus = ObpCreateObject( &NewObject, ObjectAttributes, ObjectType );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	ntStatus = ObpCreateHandle( ObpQueryCurrentHandleTable( ), NewObject, ObjectHandle );

	if ( !NT_SUCCESS( ntStatus ) ) {

		ObDereferenceObject( NewObject );

		return ntStatus;
	}

	return ntStatus;
}

NTSTATUS
ObpCreateSymbolicLink(
	__in PUNICODE_STRING LinkName,
	__in_opt PVOID Object,
	__in_opt PUNICODE_STRING LinkTargetName
)
{

	if ( Object == NULL && LinkTargetName == NULL ) {

		return STATUS_UNSUCCESSFUL;
	}

	POBJECT_ENTRY_HEADER NameLinkHeader;

	NameLinkHeader = ExAllocatePoolWithTag(
		ObjectTypeSymbolicLink->ObjectLength + sizeof( OBJECT_ENTRY_HEADER ),
		ObjectTypeSymbolicLink->PoolTag );
	ObjectTypeSymbolicLink->PoolUsage += ObjectTypeSymbolicLink->ObjectLength + sizeof( OBJECT_ENTRY_HEADER );

	KeAcquireSpinLock( &ObjectTypeSymbolicLink->ObjectList.Lock );
	if ( ObjectTypeSymbolicLink->ObjectList.List == NULL ) {

		KeInitializeListHead( &NameLinkHeader->ObjectList );
		ObjectTypeSymbolicLink->ObjectList.List = &NameLinkHeader->ObjectList;
	}
	else {

		KeInsertListEntry( ObjectTypeSymbolicLink->ObjectList.List, &NameLinkHeader->ObjectList );
	}
	KeReleaseSpinLock( &ObjectTypeSymbolicLink->ObjectList.Lock );

	NameLinkHeader->TypeDescriptor = ObjectTypeSymbolicLink;
	NameLinkHeader->ReferenceCount = 0;
	NameLinkHeader->Flags = OBJ_PERMANENT;

	POBJECT_SYMBOLIC_LINK NameLink = OB_HEADER2OBJ( NameLinkHeader );

	_memcpy( ( void* )&NameLink->LinkName, ( void* )LinkName, sizeof( UNICODE_STRING ) );

	if ( LinkTargetName != NULL ) {

		_memcpy( ( void* )&NameLink->LinkTargetName, ( void* )LinkTargetName, sizeof( UNICODE_STRING ) );
	}
	else {

		NameLink->LinkTargetName.Buffer = NULL;
		NameLink->LinkTargetName.Length = 0;
		NameLink->LinkTargetName.Size = 0;
	}

	if ( Object != NULL ) {

		NameLink->LinkTarget = OB_OBJ2HEADER( Object );
	}
	else {

		NameLink->LinkTarget = NULL;
	}

	return STATUS_SUCCESS;
}
