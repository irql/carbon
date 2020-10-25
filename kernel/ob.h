/*++

Module ObjectName:

	ob.h

Abstract:



--*/

#pragma once

/*

	okay, so to help myself write this.
	i've decided to describe the functionality.

	object type descriptor's will contain a list of every object
	of the type described.

	object entry header's contain data on an object and then the object itself
	relative to the header.

	 ________________
	|                |
	|     Object     |
	|     Header     |
	|________________|
	|                |
	|     Object     |
	|                |
	|________________|

	named object's are just their own object type, the object entries
	will be of type object symbolic link.

	handle table's are contained in kprocess structures and contain handle
	table entries and the number of total handles open.

	handle table entries contain the handle value, object and object type.

*/

typedef struct _OBJECT_SYMBOLIC_LINK {
	UNICODE_STRING LinkName;
	UNICODE_STRING LinkTargetName;	//1 |
	POBJECT_ENTRY_HEADER LinkTarget;//2

} OBJECT_SYMBOLIC_LINK, *POBJECT_SYMBOLIC_LINK;

typedef struct _HANDLE_TABLE_ENTRY {
	LIST_ENTRY HandleLinks;

	HANDLE Value;
	POBJECT_ENTRY_HEADER Object;
} HANDLE_TABLE_ENTRY, *PHANDLE_TABLE_ENTRY;

#define OB_POOL_TAG							'  bO'
#define OB_OBJ2HEADER( object )				( (POBJECT_ENTRY_HEADER)( object ) - 1 )
#define OB_HEADER2OBJ( object )				( PVOID )( (POBJECT_ENTRY_HEADER)( object ) + 1 )

NTSTATUS
ObpCreateObject(
	__out PVOID* ObjectPointer,
	__in POBJECT_ATTRIBUTES ObjectAttributes,
	__in POBJECT_TYPE_DESCRIPTOR ObjectType
);

NTSTATUS
ObpCloseHandle(
	__in PHANDLE_TABLE HandleTable,
	__in HANDLE Handle
);

NTSTATUS
ObpCreateHandle(
	__in PHANDLE_TABLE HandleTable,
	__in PVOID Object,
	__out PHANDLE Handle
);

HANDLE
ObpSupplyHandleIdentifier(
	__in PHANDLE_TABLE HandleTable
);

VOID
ObpInitializeObjectManager(

);

PHANDLE_TABLE
ObpQueryCurrentHandleTable(

);

NTSTATUS
ObpCreateSymbolicLink(
	__in PUNICODE_STRING LinkName,
	__in_opt PVOID Object,
	__in_opt PUNICODE_STRING LinkTarget
);
