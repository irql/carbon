


#include <carbsup.h>
#include "obp.h"
#include "../hal/halp.h"
#include "../ke/ki.h"
#include "../io/iop.h"
#include "../mm/mi.h"
#include "../ps/psp.h"

PLIST_ENTRY ObTypeList = NULL;

OBJECT_DIRECTORY ObRootDirectory = { RTL_CONSTANT_STRING( L"\\" ), NULL };

POBJECT_TYPE ObDirectoryObject;
POBJECT_TYPE IoSymbolicLinkObject;
POBJECT_TYPE IoDriverObject;
POBJECT_TYPE IoDeviceObject;
POBJECT_TYPE IoFileObject;
POBJECT_TYPE PsThreadObject;
POBJECT_TYPE PsProcessObject;
POBJECT_TYPE MmSectionObject;
POBJECT_TYPE KeEventObject;

NTSTATUS
ObCreateObjectType(
    _Out_    POBJECT_TYPE*   ObjectType,
    _In_     PUNICODE_STRING ObjectName,
    _In_     ULONG           Tag,
    _In_opt_ OBJECT_CLEANUP  Cleanup
)
{
    *ObjectType = MmAllocatePoolWithTag( NonPagedPoolZeroed, sizeof( OBJECT_TYPE ), OB_TAG );

    RtlCopyMemory( &( *ObjectType )->Name, ObjectName, sizeof( UNICODE_STRING ) );
    ( *ObjectType )->Tag = Tag;
    ( *ObjectType )->Cleanup = Cleanup;

    if ( ObTypeList == NULL ) {
        KeInitializeHeadList( &( *ObjectType )->TypeList );
        ObTypeList = &( *ObjectType )->TypeList;
    }
    else {
        KeInsertTailList( ObTypeList, &( *ObjectType )->TypeList );
    }

    return STATUS_SUCCESS;
}

POBJECT_DIRECTORY
ObCreateDirectory(

)
{
    OBJECT_ATTRIBUTES DirectoryAttributes = { { 0, 0, NULL }, { 0, 0, NULL }, OBJ_PERMANENT_OBJECT };
    POBJECT_DIRECTORY Directory;

    ObCreateObject( &Directory, ObDirectoryObject, &DirectoryAttributes, sizeof( OBJECT_DIRECTORY ) );

    return Directory;
}

POBJECT_DIRECTORY
ObCreateDirectoryHead(

)
{
    //
    // Directory heads are the first entries of complete directories
    // the actual directory name is inside the directory object which
    // points to this as an object.
    //

    OBJECT_ATTRIBUTES DirectoryAttributes = { { 0, 0, NULL }, { 0, 0, NULL }, OBJ_PERMANENT_OBJECT };
    POBJECT_DIRECTORY Directory;

    ObCreateObject( &Directory, ObDirectoryObject, &DirectoryAttributes, sizeof( OBJECT_DIRECTORY ) );

    Directory->Object = NULL;
    Directory->Name.Buffer = L"..";
    Directory->Name.Length = sizeof( L".." ) - 2;
    Directory->Name.MaximumLength = sizeof( L".." );

    return Directory;
}

VOID
ObInsertDirectoryEntry(
    _In_ POBJECT_DIRECTORY Directory,
    _In_ PUNICODE_STRING   Name,
    _In_ PVOID             Object
)
{
    POBJECT_DIRECTORY Last;
    //RtlDebugPrint( L"dir: %s, %s\n", Directory->Name.Buffer, Name->Buffer );
    Last = Directory->DirectoryLink;

    if ( Directory->DirectoryLink == NULL ) {
        Directory->DirectoryLink = ObCreateDirectory( );
        Directory->DirectoryLink->DirectoryLink = NULL;
        Directory->DirectoryLink->Object = Object;
        RtlCopyMemory( &Directory->DirectoryLink->Name, Name, sizeof( UNICODE_STRING ) );
    }
    else {

        while ( Last->DirectoryLink != NULL ) {
            Last = Last->DirectoryLink;
        }
        Last->DirectoryLink = ObCreateDirectory( );
        Last->DirectoryLink->DirectoryLink = NULL;
        Last->DirectoryLink->Object = Object;
        RtlCopyMemory( &Last->DirectoryLink->Name, Name, sizeof( UNICODE_STRING ) );
    }
}

VOID
ObInitializeObjectManager(

)
{
    UNICODE_STRING ObjectDirectoryName = RTL_CONSTANT_STRING( L"Directory" );
    UNICODE_STRING ObjectSymbolicLinkName = RTL_CONSTANT_STRING( L"SymbolicLink" );
    UNICODE_STRING ObjectDriverName = RTL_CONSTANT_STRING( L"Driver" );
    UNICODE_STRING ObjectDeviceName = RTL_CONSTANT_STRING( L"Device" );
    UNICODE_STRING ObjectThreadName = RTL_CONSTANT_STRING( L"Thread" );
    UNICODE_STRING ObjectEventName = RTL_CONSTANT_STRING( L"Event" );
    UNICODE_STRING ObjectFileName = RTL_CONSTANT_STRING( L"File" );
    UNICODE_STRING ObjectSectionName = RTL_CONSTANT_STRING( L"Section" );
    UNICODE_STRING ObjectMutexName = RTL_CONSTANT_STRING( L"Mutex" );
    UNICODE_STRING ObjectInterruptName = RTL_CONSTANT_STRING( L"Interrupt" );

    UNICODE_STRING DeviceDirectory = RTL_CONSTANT_STRING( L"Device" );
    UNICODE_STRING GlobalDirectory = RTL_CONSTANT_STRING( L"??" );
    UNICODE_STRING DriverDirectory = RTL_CONSTANT_STRING( L"Driver" );
    UNICODE_STRING KnownDllsDirectory = RTL_CONSTANT_STRING( L"KnownDlls" );

    //
    // Implement object destroy callbacks for cleaning up 
    // extra buffers, like thread stacks and device extensions
    //

    ObCreateObjectType( &ObDirectoryObject, &ObjectDirectoryName, 'eriD', NULL );
    ObCreateObjectType( &IoSymbolicLinkObject, &ObjectSymbolicLinkName, 'kniL', NULL );
    ObCreateObjectType( &IoDriverObject, &ObjectDriverName, 'virD', NULL );
    ObCreateObjectType( &PsThreadObject, &ObjectThreadName, 'erhT', ( OBJECT_CLEANUP )PspCleanupThread );
    ObCreateObjectType( &PsProcessObject, &ObjectThreadName, 'corP', NULL );
    ObCreateObjectType( &IoDeviceObject, &ObjectDeviceName, 'iveD', ( OBJECT_CLEANUP )IopCleanupDevice );
    ObCreateObjectType( &KeEventObject, &ObjectEventName, 'nevE', NULL );
    ObCreateObjectType( &IoFileObject, &ObjectFileName, 'eliF', ( OBJECT_CLEANUP )IopCleanupFile );
    ObCreateObjectType( &MmSectionObject, &ObjectSectionName, 'tceS', ( OBJECT_CLEANUP )MiCleanupSection );
    ObCreateObjectType( &KeMutexObject, &ObjectMutexName, 'etuM', NULL );
    ObCreateObjectType( &IoInterruptObject, &ObjectInterruptName, ' tnI', NULL );

    ObInsertDirectoryEntry( &ObRootDirectory, &DeviceDirectory, ObCreateDirectoryHead( ) );
    ObInsertDirectoryEntry( &ObRootDirectory, &GlobalDirectory, ObCreateDirectoryHead( ) );
    ObInsertDirectoryEntry( &ObRootDirectory, &DriverDirectory, ObCreateDirectoryHead( ) );
    ObInsertDirectoryEntry( &ObRootDirectory, &KnownDllsDirectory, ObCreateDirectoryHead( ) );
}
