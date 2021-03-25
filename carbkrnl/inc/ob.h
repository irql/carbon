


#pragma once

#define OBJ_PERMANENT_OBJECT    0x80
#define OBJ_EXCLUSIVE_OBJECT    0x40
#define OBJ_KERNEL_HANDLE       0x20

typedef struct _OBJECT_DIRECTORY {
    UNICODE_STRING    Name;
    PVOID             Object;
    POBJECT_DIRECTORY DirectoryLink;

} OBJECT_DIRECTORY, *POBJECT_DIRECTORY;

typedef VOID( *OBJECT_CLEANUP )(
    _In_ PVOID Object
    );

typedef struct _OBJECT_TYPE {
    UNICODE_STRING      Name;
    ULONG32             Tag;
    ULONG64             ObjectCount;
    ULONG64             HandleCount;
    ULONG64             PointerCount;
    ULONG64             PoolUsage;
    OBJECT_CLEANUP      Cleanup;

    LIST_ENTRY          TypeList;
    KSPIN_LOCK          ObjectListLock;
    PLIST_ENTRY         ObjectList;
} OBJECT_TYPE, *POBJECT_TYPE;

typedef struct _HANDLE_TABLE {
    ULONG64     HandleCount;
    PLIST_ENTRY HandleLinks;
    KSPIN_LOCK  HandleLock;

} HANDLE_TABLE, *PHANDLE_TABLE;

NTSYSAPI EXTERN OBJECT_DIRECTORY ObRootDirectory;

NTSYSAPI EXTERN POBJECT_TYPE ObDirectoryObject;

NTSYSAPI
NTSTATUS
ObCreateObjectType(
    _Out_    POBJECT_TYPE*   ObjectType,
    _In_     PUNICODE_STRING ObjectName,
    _In_     ULONG           Tag,
    _In_opt_ OBJECT_CLEANUP  Cleanup
);

NTSYSAPI
NTSTATUS
ObCreateObject(
    _Out_ PVOID*             Object,
    _In_  POBJECT_TYPE       ObjectType,
    _In_  POBJECT_ATTRIBUTES ObjectAttributes,
    _In_  ULONG64            Length
);

NTSYSAPI
NTSTATUS
ObReferenceObjectByName(
    _Out_    PVOID*          Object,
    _In_     PUNICODE_STRING ObjectName,
    _In_opt_ POBJECT_TYPE    Type
);

NTSYSAPI
NTSTATUS
ObReferenceObjectByHandle(
    _Out_    PVOID*          Object,
    _In_     HANDLE          Handle,
    _In_     ACCESS_MASK     Access,
    _In_     KPROCESSOR_MODE AccessMode,
    _In_opt_ POBJECT_TYPE    Type
);

#define NtCurrentProcess( ) ( ( HANDLE )( -1 ) )
#define NtCurrentThread( )  ( ( HANDLE )( -2 ) )

NTSYSAPI
VOID
ObDereferenceObject(
    _In_ PVOID Object
);

NTSYSAPI
NTSTATUS
ObOpenObjectFromPointer(
    _Out_ PHANDLE         ObjectHandle,
    _In_  PVOID           Object,
    _In_  ACCESS_MASK     Access,
    _In_  ULONG           HandleAttributes,
    _In_  KPROCESSOR_MODE AccessMode
);

NTSYSAPI
NTSTATUS
ObQueryHandleAccess(
    _Out_ PACCESS_MASK Access,
    _In_  HANDLE       Handle
);

typedef BOOLEAN( *PQUERY_PROCEDURE )(
    _In_ POBJECT_TYPE Type,
    _In_ PVOID        Object,
    _In_ PVOID        Context
    );

NTSYSAPI
VOID
ObQueryObjectList(
    _In_ POBJECT_TYPE     Type,
    _In_ PQUERY_PROCEDURE Procedure,
    _In_ PVOID            Context
);

NTSYSAPI
NTSTATUS
ObCloseHandle(
    _In_ HANDLE Handle
);

NTSYSAPI
VOID
ObReferenceObject(
    _In_ PVOID Object
);

NTSYSAPI
NTSTATUS
ZwClose(
    _In_ HANDLE Handle
);

NTSTATUS
NtClose(
    _In_ HANDLE Handle
);

NTSYSAPI
VOID
ObInsertDirectoryEntry(
    _In_ POBJECT_DIRECTORY Directory,
    _In_ PUNICODE_STRING   Name,
    _In_ PVOID             Object
);

NTSYSAPI
POBJECT_DIRECTORY
ObCreateDirectoryHead(

);

NTSYSAPI
NTSTATUS
ZwOpenDirectoryObject(
    _Out_ PHANDLE            DirectoryHandle,
    _In_  ACCESS_MASK        DesiredAccess,
    _In_  POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
ZwQueryDirectoryObject(
    _In_      HANDLE   DirectoryHandle,
    _Out_     PVOID    Buffer,
    _In_      ULONG64  Length,
    _Out_opt_ PULONG64 ReturnLength
);

//
// Shouldn't have the obp prefix or shouldn't
// be exported, although it is a useful rtl api.
//

NTSYSAPI
NTSTATUS
ObpDecomposeDirectory(
    _In_  PUNICODE_STRING Directory,
    _Out_ PWSTR**         Decomposed
);
