/*++

Module ObjectName:

	obquery.c

Abstract:

	Object querying.

--*/

#include <carbsup.h>
#include "ob.h"
#include "ke.h"

PHANDLE_TABLE
ObpQueryCurrentHandleTable(

)
{

	return &KiQueryCurrentProcess( )->ProcessHandleTable;
}

VOID
ObReferenceObject(
	__in PVOID Object
)
{
	POBJECT_ENTRY_HEADER ObjectHeader = OB_OBJ2HEADER( Object );

	ObjectHeader->ReferenceCount++;

	return;
}

VOID
ObDereferenceObject(
	__in PVOID Object
)
{
	POBJECT_ENTRY_HEADER ObjectHeader = OB_OBJ2HEADER( Object );

	if ( ObjectHeader->Flags & OBJ_PERMANENT ) {

		return;
	}

	ObjectHeader->ReferenceCount--;

	if ( ObjectHeader->ReferenceCount == 0 ) {

		ObDestroyObject( ObjectHeader );


	}

	return;
}

NTSTATUS
ObReferenceObjectByName(
	__in PUNICODE_STRING ObjectName,
	__out PVOID* Object
)
{
	KeAcquireSpinLock( &ObjectTypeSymbolicLink->ObjectList.Lock );

	PLIST_ENTRY Flink = ObjectTypeSymbolicLink->ObjectList.List;
	do {
		POBJECT_ENTRY_HEADER NameHeader = CONTAINING_RECORD( Flink, OBJECT_ENTRY_HEADER, ObjectList );
		POBJECT_SYMBOLIC_LINK NameLink = OB_HEADER2OBJ( NameHeader );

		if ( NameLink->LinkName.Buffer != NULL && NameLink->LinkTarget != NULL ) {

			if ( RtlUnicodeStringCompare( &NameLink->LinkName, ObjectName ) == 0 ) {

				NameLink->LinkTarget->ReferenceCount++;
				*Object = ( PVOID )( NameLink->LinkTarget + 1 );

				KeReleaseSpinLock( &ObjectTypeSymbolicLink->ObjectList.Lock );
				return STATUS_SUCCESS;
			}
		}

		Flink = Flink->Flink;
	} while ( Flink != ObjectTypeSymbolicLink->ObjectList.List );

	KeReleaseSpinLock( &ObjectTypeSymbolicLink->ObjectList.Lock );

	return STATUS_NOT_FOUND;
}

NTSTATUS
ObReferenceObjectByHandle(
	__in HANDLE Handle,
	__out PVOID* Object
)
{

	PHANDLE_TABLE HandleTable = ObpQueryCurrentHandleTable( );

	KeAcquireSpinLock( &HandleTable->HandleLinks.Lock );

	PLIST_ENTRY Flink = HandleTable->HandleLinks.List;
	do {
		PHANDLE_TABLE_ENTRY HandleEntry = CONTAINING_RECORD( Flink, HANDLE_TABLE_ENTRY, HandleLinks );

		if ( HandleEntry->Value == Handle ) {

			*Object = OB_HEADER2OBJ( HandleEntry->Object );

			KeReleaseSpinLock( &HandleTable->HandleLinks.Lock );
			return STATUS_SUCCESS;
		}

		Flink = Flink->Flink;
	} while ( Flink != HandleTable->HandleLinks.List );

	KeReleaseSpinLock( &HandleTable->HandleLinks.Lock );

	return STATUS_NOT_FOUND;
}

NTSTATUS
ObParseObjectDirectory(
	__in PUNICODE_STRING EntireName,
	__out PUNICODE_STRING ObjectName,
	__out PUNICODE_STRING RootName,
	__out_opt PVOID* Object
)
/*++

Routine Description:

	This function take's the EntireName parameter and parses it
	through getting the FIRST symbolic link prefix match. This is okay
	since system driver's are initialized first.

Arguments:

	abc.

Return Value:

	Status.

--*/
{

	KeAcquireSpinLock( &ObjectTypeSymbolicLink->ObjectList.Lock );

	PLIST_ENTRY Flink = ObjectTypeSymbolicLink->ObjectList.List;
	do {
		POBJECT_ENTRY_HEADER NameHeader = CONTAINING_RECORD( Flink, OBJECT_ENTRY_HEADER, ObjectList );
		POBJECT_SYMBOLIC_LINK NameLink = OB_HEADER2OBJ( NameHeader );

		if ( NameLink->LinkTarget == NULL && NameLink->LinkName.Buffer != NULL && NameLink->LinkTargetName.Buffer != NULL ) {

			if ( RtlUnicodeStringCompareLength( &NameLink->LinkName, EntireName, NameLink->LinkName.Length ) == 0 ) {
				//reconstruct the string, replacing Old with New.

				ULONG32 NeededLength = ( NameLink->LinkTargetName.Length + ( EntireName->Length - NameLink->LinkName.Length ) + 1 ) * sizeof( WCHAR );

				if ( NeededLength > EntireName->MaximumLength ) {

					PWCHAR NewBuffer = ( PWCHAR )ExAllocatePoolWithTag( NeededLength, ' rtS' );
					_memcpy( ( void* )NewBuffer, ( void* )EntireName->Buffer, EntireName->MaximumLength );
					ExFreePoolWithTag( EntireName->Buffer, ' rtS' );
					EntireName->Buffer = NewBuffer;
					EntireName->MaximumLength = ( USHORT )NeededLength;
				}

				PWCHAR TempBuffer = NULL;
				ULONG32 TempBufferSize = ( EntireName->Length - NameLink->LinkName.Length ) * sizeof( WCHAR );

				if ( TempBufferSize > 0 ) {

					TempBuffer = ( PWCHAR )ExAllocatePoolWithTag( TempBufferSize, ' rtS' );
					_memcpy( ( void* )TempBuffer, ( void* )( EntireName->Buffer + NameLink->LinkName.Length ), TempBufferSize );
				}

				_memset( ( void* )EntireName->Buffer, 0, EntireName->MaximumLength );
				_memcpy( ( void* )EntireName->Buffer, ( void* )NameLink->LinkTargetName.Buffer, NameLink->LinkTargetName.Length * sizeof( WCHAR ) );

				if ( TempBufferSize > 0 ) {

					_memcpy( ( void* )( EntireName->Buffer + NameLink->LinkTargetName.Length ), ( void* )TempBuffer, TempBufferSize );
					*( EntireName->Buffer + NameLink->LinkTargetName.Length + ( TempBufferSize / sizeof( WCHAR ) ) ) = 0;
				}
			}
		}

		Flink = Flink->Flink;
	} while ( Flink != ObjectTypeSymbolicLink->ObjectList.List );

	do {
		POBJECT_ENTRY_HEADER NameHeader = CONTAINING_RECORD( Flink, OBJECT_ENTRY_HEADER, ObjectList );
		POBJECT_SYMBOLIC_LINK NameLink = OB_HEADER2OBJ( NameHeader );

		if ( NameLink->LinkTarget != NULL && NameLink->LinkName.Buffer != NULL/* && NameLink->LinkTarget->TypeDescriptor == ObjectTypeDevice*/ ) {

			if ( RtlUnicodeStringCompareLength( &NameLink->LinkName, EntireName, NameLink->LinkName.Length ) == 0 ) {

				_memcpy( ObjectName, &NameLink->LinkName, sizeof( UNICODE_STRING ) );
				_memcpy( RootName, EntireName, sizeof( UNICODE_STRING ) );
				RootName->Buffer += NameLink->LinkName.Length;
				RootName->Length -= NameLink->LinkName.Length;
				RootName->MaximumLength -= ( NameLink->LinkName.Length * sizeof( WCHAR ) );

				if ( Object ) {
					NameLink->LinkTarget->ReferenceCount++;
					*Object = ( PVOID )( NameLink->LinkTarget + 1 );
				}

				KeReleaseSpinLock( &ObjectTypeSymbolicLink->ObjectList.Lock );
				return STATUS_SUCCESS;
			}
		}

		Flink = Flink->Flink;
	} while ( Flink != ObjectTypeSymbolicLink->ObjectList.List );

	KeReleaseSpinLock( &ObjectTypeSymbolicLink->ObjectList.Lock );
	return STATUS_NOT_FOUND;
}

NTSTATUS
ObQueryObjectType(
	__in PVOID Object,
	__out POBJECT_TYPE_DESCRIPTOR* Type
)
{
	POBJECT_ENTRY_HEADER ObjectHeader = OB_OBJ2HEADER( Object );
	*Type = ObjectHeader->TypeDescriptor;

	return STATUS_SUCCESS;
}
