




#pragma once

typedef struct _HANDLE_TABLE {
	ULONG64 TotalNumberOfHandles;
	KLOCKED_LIST HandleLinks;
} HANDLE_TABLE, *PHANDLE_TABLE;

#define OBJ_PERMANENT						(0x00000001L)

typedef struct _OBJECT_ATTRIBUTES {
	ULONG32 Attributes;
	PUNICODE_STRING ObjectName;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

typedef struct _OBJECT_TYPE_DESCRIPTOR {
	ULONG32 ObjectLength;

	LIST_ENTRY TypeList;
	UNICODE_STRING ObjectName;

	ULONG32 TotalNumberOfObjects;
	ULONG32 TotalNumberOfHandles;

	ULONG32 PoolTag;
	ULONG32 PoolUsage;

	//PLIST_ENTRY ObjectList;
	KLOCKED_LIST ObjectList;
} OBJECT_TYPE_DESCRIPTOR, *POBJECT_TYPE_DESCRIPTOR;

typedef struct _OBJECT_ENTRY_HEADER {
	//add this
	//ULONG32 ObjectTag;

	POBJECT_TYPE_DESCRIPTOR TypeDescriptor;
	LIST_ENTRY ObjectList;
	ULONG32 ReferenceCount;
	ULONG32 Flags;

	// the actual object will be here.
	// eg header + 1 is the addr.

} OBJECT_ENTRY_HEADER, *POBJECT_ENTRY_HEADER;

NTSYSAPI EXTERN POBJECT_TYPE_DESCRIPTOR ObjectTypeThread;
NTSYSAPI EXTERN POBJECT_TYPE_DESCRIPTOR ObjectTypeDriver;
NTSYSAPI EXTERN POBJECT_TYPE_DESCRIPTOR ObjectTypeDevice;
NTSYSAPI EXTERN POBJECT_TYPE_DESCRIPTOR ObjectTypeModule;
NTSYSAPI EXTERN POBJECT_TYPE_DESCRIPTOR ObjectTypeProcess;
NTSYSAPI EXTERN POBJECT_TYPE_DESCRIPTOR ObjectTypeSymbolicLink;
NTSYSAPI EXTERN POBJECT_TYPE_DESCRIPTOR ObjectTypeIoCommunicationBlock;

NTSYSAPI
VOID
ObReferenceObject(
	__in PVOID Object
);

NTSYSAPI
VOID
ObDereferenceObject(
	__in PVOID Object
);

NTSYSAPI
NTSTATUS
ObReferenceObjectByName(
	__in PUNICODE_STRING ObjectName,
	__out PVOID* Object
);

NTSYSAPI
NTSTATUS
ObReferenceObjectByHandle(
	__in HANDLE Handle,
	__out PVOID* Object
);

NTSYSAPI
NTSTATUS
ObCreateHandle(
	__out PHANDLE Handle,
	__in PVOID Object
);

NTSYSAPI
NTSTATUS
ObCloseHandle(
	__in HANDLE Handle
);

NTSYSAPI
NTSTATUS
ObParseObjectDirectory(
	__in PUNICODE_STRING EntireName,
	__out PUNICODE_STRING ObjectName,
	__out PUNICODE_STRING RootName,
	__out_opt PVOID* Object
);

NTSYSAPI
NTSTATUS
ObQueryObjectType(
	__in PVOID Object,
	__out POBJECT_TYPE_DESCRIPTOR* Type
);

NTSYSAPI
NTSTATUS
ObInitializeObjectType(
	__in PWCHAR ObjectName,
	__in ULONG32 ObjectLength,
	__in ULONG32 AllocationTag,
	__out POBJECT_TYPE_DESCRIPTOR* ObjectType
);

NTSYSAPI
NTSTATUS
ObpCreateObject(
	__out PVOID* ObjectPointer,
	__in POBJECT_ATTRIBUTES ObjectAttributes,
	__in POBJECT_TYPE_DESCRIPTOR ObjectType
);

NTSYSAPI
NTSTATUS
ObDestroyObject(
	__in PVOID Object
);