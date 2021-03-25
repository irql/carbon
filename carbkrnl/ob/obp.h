


#pragma once

#define OB_TAG '  bO'

typedef struct _OBJECT_HEADER {
    LIST_ENTRY   ObjectList;
    ULONG64      PointerCount;
    ULONG64      HandleCount;
    ULONG64      Length;
    ULONG32      Flags;
    POBJECT_TYPE Type;

} OBJECT_HEADER, *POBJECT_HEADER;

// these 2 need api's.

typedef struct _HANDLE_TABLE_ENTRY {
    LIST_ENTRY  HandleLinks;
    HANDLE      Handle;
    PVOID       Object;
    ACCESS_MASK Access;
} HANDLE_TABLE_ENTRY, *PHANDLE_TABLE_ENTRY;

#define ObpGetHeaderFromObject( object )    ( ( POBJECT_HEADER )( object ) - 1 )
#define ObpGetObjectFromHeader( header )    ( ( POBJECT_HEADER )( header ) + 1 )

#define ObpIncrementHandleCount( object )   ObpGetHeaderFromObject( object )->HandleCount++, ObpGetHeaderFromObject( object )->Type->HandleCount++
#define ObpIncrementPointerCount( object )  ObpGetHeaderFromObject( object )->PointerCount++, ObpGetHeaderFromObject( object )->Type->PointerCount++
#define ObpDecrementHandleCount( object )   ObpGetHeaderFromObject( object )->HandleCount--, ObpGetHeaderFromObject( object )->Type->HandleCount--
#define ObpDecrementPointerCount( object )  ObpGetHeaderFromObject( object )->PointerCount--, ObpGetHeaderFromObject( object )->Type->PointerCount--

NTSTATUS
ObParseDirectory(
    _In_  PUNICODE_STRING    ObjectName,
    _Out_ PVOID*             Object
);

NTSTATUS
ObParseCreateDirectory(
    _In_  PUNICODE_STRING    ObjectName,
    _Out_ POBJECT_DIRECTORY* Parent
);

VOID
ObInitializeObjectManager(

);

NTSTATUS
ObDestroyObject(
    _In_ PVOID Object
);