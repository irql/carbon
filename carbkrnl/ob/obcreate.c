


#include <carbsup.h>
#include "obp.h"

NTSTATUS
ObpDecomposeDirectory(
    _In_  PUNICODE_STRING Directory,
    _Out_ PWSTR**         Decomposed
)
{
    //
    // potentially: add a return length parameter, and depend
    // on the caller for the decomposed buffer.
    //

    ULONG DirectoryLength;
    ULONG BufferLength;
    ULONG DecomposedIndex;
    ULONG i;

    DirectoryLength = Directory->Length / 2;
    BufferLength = sizeof( PWSTR );

    if ( DirectoryLength == 0 ) {
        return STATUS_INVALID_PATH;
    }

    if ( Directory->Buffer[ 0 ] != '\\' ) {
        return STATUS_INVALID_PATH;
    }

    if ( Directory->Buffer[ 1 ] == 0 ) {
        *Decomposed = ( PWSTR* )MmAllocatePoolWithTag( NonPagedPool, BufferLength, OB_TAG );
        ( *Decomposed )[ 0 ] = NULL;
        return STATUS_SUCCESS;
    }

    for ( i = 1; i < DirectoryLength; i++ ) {

        if ( Directory->Buffer[ i ] == '\\' &&
             Directory->Buffer[ i + 1 ] == 0 ) {
            break;
        }

        if ( Directory->Buffer[ i ] == '\\' ) {
            BufferLength += sizeof( PWSTR );

            if ( Directory->Buffer[ i + 1 ] == '\\' ) {
                return STATUS_INVALID_PATH;
            }
        }
    }

    *Decomposed = ( PWSTR* )MmAllocatePoolWithTag( NonPagedPool, BufferLength, OB_TAG );

    for ( i = 0, DecomposedIndex = 0; i < DirectoryLength; i++ ) {

        if ( Directory->Buffer[ i ] == '\\' &&
             Directory->Buffer[ i + 1 ] == 0 ) {
            Directory->Buffer[ i ] = 0;
            break;
        }

        if ( Directory->Buffer[ i ] == '\\' ) {
            ( *Decomposed )[ DecomposedIndex++ ] = &Directory->Buffer[ i + 1 ];
            Directory->Buffer[ i ] = 0;
        }
    }

    ( *Decomposed )[ DecomposedIndex ] = NULL;
    return STATUS_SUCCESS;
}

NTSTATUS
ObParseDirectory(
    _In_  PUNICODE_STRING    ObjectName,
    _Out_ PVOID*             Object
)
{
    NTSTATUS ntStatus;
    PWSTR* Decomposed;
    ULONG CurrentDirectory;
    POBJECT_DIRECTORY ObjectDirectory;
    POBJECT_DIRECTORY CurrentEntry;

    ntStatus = ObpDecomposeDirectory( ObjectName, &Decomposed );

    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    CurrentDirectory = 0;
    ObjectDirectory = &ObRootDirectory;
    while ( Decomposed[ CurrentDirectory ] != NULL ) {

        CurrentEntry = ObjectDirectory->DirectoryLink;
        while ( CurrentEntry != NULL ) {

            if ( ( ObpGetHeaderFromObject( CurrentEntry->Object )->Type == ObDirectoryObject ||
                   Decomposed[ CurrentDirectory + 1 ] == NULL ) &&
                 lstrcmpiW( CurrentEntry->Name.Buffer, Decomposed[ CurrentDirectory ] ) == 0 ) {
                //RtlDebugPrint( L"Traversed %s\n", Decomposed[ CurrentDirectory ] );
                ObjectDirectory = CurrentEntry->Object;
                CurrentDirectory++;
                break;
            }
            CurrentEntry = CurrentEntry->DirectoryLink;
        }

        if ( CurrentEntry == NULL ) {

            MmFreePoolWithTag( Decomposed, OB_TAG );
            return STATUS_INVALID_PATH;
        }
    }

    if ( ObpGetHeaderFromObject( ObjectDirectory )->Type == IoSymbolicLinkObject ) {

        //RtlDebugPrint( L"symbolic reparse %s\n", ( ( POBJECT_SYMBOLIC_LINK )ObjectDirectory )->LinkTarget.Buffer );
        MmFreePoolWithTag( Decomposed, OB_TAG );
        RtlCopyMemory(
            ObjectName->Buffer,
            ( ( POBJECT_SYMBOLIC_LINK )ObjectDirectory )->LinkTarget.Buffer,
            ( ULONG64 )( ( POBJECT_SYMBOLIC_LINK )ObjectDirectory )->LinkTarget.Length + 2 );
        ObjectName->Length = ( ( POBJECT_SYMBOLIC_LINK )ObjectDirectory )->LinkTarget.Length;
        return ObParseDirectory( ObjectName, Object );
    }

    *Object = ObjectDirectory;
    MmFreePoolWithTag( Decomposed, OB_TAG );
    return STATUS_SUCCESS;
}

NTSTATUS
ObParseCreateDirectory(
    _In_  PUNICODE_STRING    ObjectName,
    _Out_ POBJECT_DIRECTORY* Parent
)
{
    //
    // Parent is the highest directory
    // in the path parsed, and the object
    // name's buffer specifies the name of 
    // new path to be created.
    //

    NTSTATUS ntStatus;
    PWSTR* Decomposed;
    ULONG CurrentDirectory;
    POBJECT_DIRECTORY ObjectDirectory;
    POBJECT_DIRECTORY CurrentEntry;

    ntStatus = ObpDecomposeDirectory( ObjectName, &Decomposed );

    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    if ( Decomposed[ 0 ] == NULL ) {

        MmFreePoolWithTag( Decomposed, OB_TAG );
        return STATUS_INVALID_PATH;
    }

    ObjectName->Buffer[ 0 ] = 0;
    CurrentDirectory = 0;
    ObjectDirectory = &ObRootDirectory;
    while ( Decomposed[ CurrentDirectory ] != NULL ) {

        CurrentEntry = ObjectDirectory->DirectoryLink;
        while ( CurrentEntry != NULL ) {

            if ( ObpGetHeaderFromObject( CurrentEntry->Object )->Type == ObDirectoryObject &&
                 lstrcmpiW( CurrentEntry->Name.Buffer, Decomposed[ CurrentDirectory ] ) == 0 ) {
                //RtlDebugPrint( L"Travers3d %s\n", Decomposed[ CurrentDirectory ] );
                ObjectDirectory = CurrentEntry->Object;
                CurrentDirectory++;
                break;
            }
            CurrentEntry = CurrentEntry->DirectoryLink;
        }

        if ( CurrentEntry == NULL && Decomposed[ CurrentDirectory + 1 ] == NULL ) {

            //
            // If nothing has matched and we've reached the last
            // string in the list, we can assume this is the new directory name.
            //

            *Parent = ObjectDirectory;
            ObjectName->Length = ( USHORT )RtlStringLength( Decomposed[ CurrentDirectory ] ) * sizeof( WCHAR );
            RtlMoveMemory( ObjectName->Buffer, Decomposed[ CurrentDirectory ], ( ULONG )ObjectName->Length + 2 );
            break;
        }

        if ( CurrentEntry == NULL ) {

            MmFreePoolWithTag( Decomposed, OB_TAG );
            return STATUS_INVALID_PATH;
        }
    }

    *Parent = ObjectDirectory;
    MmFreePoolWithTag( Decomposed, OB_TAG );
    return STATUS_SUCCESS;

}

NTSTATUS
ObCreateObject(
    _Out_ PVOID*             Object,
    _In_  POBJECT_TYPE       ObjectType,
    _In_  POBJECT_ATTRIBUTES ObjectAttributes,
    _In_  ULONG64            Length
)
{
    NTSTATUS ntStatus;
    POBJECT_HEADER ObjectHeader;
    UNICODE_STRING DuplicateName;
    POBJECT_DIRECTORY ObjectDirectory;
    KIRQL PreviousIrql;

    ObjectDirectory = NULL;
    if ( ObjectAttributes->ObjectName.Buffer != NULL ) {

        DuplicateName.Length = ObjectAttributes->ObjectName.Length;
        DuplicateName.MaximumLength = ObjectAttributes->ObjectName.MaximumLength;
        DuplicateName.Buffer = MmAllocatePoolWithTag( NonPagedPool, DuplicateName.MaximumLength, OB_TAG );
        RtlCopyMemory( DuplicateName.Buffer, ObjectAttributes->ObjectName.Buffer, DuplicateName.MaximumLength );
        ntStatus = ObParseCreateDirectory( &DuplicateName, &ObjectDirectory );

        if ( !NT_SUCCESS( ntStatus ) ) {

            return ntStatus;
        }

        if ( DuplicateName.Buffer[ 0 ] == 0 ) {

            return STATUS_INVALID_PATH;
        }
    }

    ObjectHeader = MmAllocatePoolWithTag( NonPagedPoolZeroed, sizeof( OBJECT_HEADER ) + Length, ObjectType->Tag );
    ObjectType->PoolUsage += sizeof( OBJECT_HEADER ) + Length;

    *Object = ObpGetObjectFromHeader( ObjectHeader );

    ObjectHeader->Type = ObjectType;
    ObjectHeader->Flags = ObjectAttributes->Attributes;
    ObjectHeader->PointerCount = 1;
    ObjectHeader->Length = Length;

    if ( ObjectDirectory != NULL ) {

        //RtlDebugPrint( L"Named object: %s\n", DuplicateName.Buffer );
        ObInsertDirectoryEntry( ObjectDirectory, &DuplicateName, ObpGetObjectFromHeader( ObjectHeader ) );
    }

    KeAcquireSpinLock( &ObjectType->ObjectListLock, &PreviousIrql );
    if ( ObjectType->ObjectList == NULL ) {
        KeInitializeHeadList( &ObjectHeader->ObjectList );
        ObjectType->ObjectList = &ObjectHeader->ObjectList;
    }
    else {
        KeInsertTailList( ObjectType->ObjectList, &ObjectHeader->ObjectList );
    }
    ObjectType->ObjectCount++;
    KeReleaseSpinLock( &ObjectType->ObjectListLock, PreviousIrql );

    return STATUS_SUCCESS;
}

NTSTATUS
ObDestroyObject(
    _In_ PVOID Object
)
{
    POBJECT_TYPE ObjectType;
    POBJECT_HEADER ObjectHeader;
    KIRQL PreviousIrql;

    ObjectHeader = ObpGetHeaderFromObject( Object );
    ObjectType = ObjectHeader->Type;

    //RtlDebugPrint( L"[object] DESTROYED object of type: %s %ull\n", ObjectType->Name.Buffer, ObjectHeader );

    if ( ObjectType->Cleanup != NULL ) {

        ObjectType->Cleanup( Object );
    }

    KeAcquireSpinLock( &ObjectType->ObjectListLock, &PreviousIrql );
    ObjectType->ObjectCount--;
    ObjectType->PoolUsage -= sizeof( OBJECT_HEADER ) + ObjectHeader->Length;
#if 1
    if ( ObjectType->ObjectCount == 0 ) {
        ObjectType->ObjectList = NULL;
    }
    else {
        KeRemoveList( &ObjectHeader->ObjectList );
    }
#endif
    KeReleaseSpinLock( &ObjectType->ObjectListLock, PreviousIrql );

    MmFreePoolWithTag( ObjectHeader, ObjectType->Tag );

    return STATUS_SUCCESS;
}
