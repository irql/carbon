


#include <carbsup.h>
#include "obp.h"
#include "../hal/halp.h"
#include "../ke/ki.h"

NTSTATUS
ObReferenceObjectByHandle(
    _Out_    PVOID*          Object,
    _In_     HANDLE          Handle,
    _In_     ACCESS_MASK     Access,
    _In_     KPROCESSOR_MODE AccessMode,
    _In_opt_ POBJECT_TYPE    Type
)
{
    PKPROCESS Process;
    PLIST_ENTRY Flink;
    PHANDLE_TABLE_ENTRY Entry;
    KIRQL PreviousIrql;

    if ( Handle == NtCurrentProcess( ) ) {

        *Object = PsGetCurrentProcess( );
        ObpIncrementPointerCount( *Object );
        return STATUS_SUCCESS;
    }

    if ( Handle == NtCurrentThread( ) ) {

        *Object = PsGetCurrentThread( );
        ObpIncrementPointerCount( *Object );
        return STATUS_SUCCESS;
    }

    if ( Handle < 0 && AccessMode == KernelMode ) {

        //
        // something to consider: bypass access checks with kernel handles by 
        // returning -1 for their access?
        //

        Process = PsInitialSystemProcess;
    }
    else {

        Process = PsGetCurrentProcess( );
    }

    KeAcquireSpinLock( &Process->HandleTable.HandleLock, &PreviousIrql );
    Flink = Process->HandleTable.HandleLinks;
    do {
        Entry = CONTAINING_RECORD( Flink, HANDLE_TABLE_ENTRY, HandleLinks );

        if ( Entry->Handle == Handle ) {

            if ( Type != NULL && ObpGetHeaderFromObject( Entry->Object )->Type != Type ) {

                KeReleaseSpinLock( &Process->HandleTable.HandleLock, PreviousIrql );
                return STATUS_INVALID_OBJECT_TYPE;
            }

            if ( ( Entry->Access & Access ) != Access ) {

                KeReleaseSpinLock( &Process->HandleTable.HandleLock, PreviousIrql );
                return STATUS_ACCESS_DENIED;
            }

            *Object = Entry->Object;
            ObpIncrementPointerCount( Entry->Object );
            KeReleaseSpinLock( &Process->HandleTable.HandleLock, PreviousIrql );
            return STATUS_SUCCESS;
        }

        Flink = Flink->Flink;
    } while ( Flink != Process->HandleTable.HandleLinks );

    KeReleaseSpinLock( &Process->HandleTable.HandleLock, PreviousIrql );
    return STATUS_INVALID_HANDLE;
}

VOID
ObDereferenceObject(
    _In_ PVOID Object
)
{
    //
    // TODO: implement a queue for object destruction
    //

    POBJECT_TYPE ObjectType;
    POBJECT_HEADER Header;

    Header = ObpGetHeaderFromObject( Object );
    ObjectType = Header->Type;

    //RtlDebugPrint( L"[object] dereference of %ull originating from %ull\n", Object, _ReturnAddress( ) );

    ObpDecrementPointerCount( Object );

    if ( Header->PointerCount == 0 &&
         Header->HandleCount == 0 &&
         ( Header->Flags & OBJ_PERMANENT_OBJECT ) == 0 ) {

        // disabled because of sect-vad-file
        ObDestroyObject( Object );
    }

}

VOID
ObReferenceObject(
    _In_ PVOID Object
)
{
    ObpIncrementPointerCount( Object );
}

NTSTATUS
ObReferenceObjectByName(
    _Out_    PVOID*          Object,
    _In_     PUNICODE_STRING ObjectName,
    _In_opt_ POBJECT_TYPE    Type
)
{
    NTSTATUS ntStatus;
    UNICODE_STRING DuplicateName;
    PVOID OriginalDuplicate;
    PVOID ObjectParsed;

    DuplicateName.Length = ObjectName->Length;
    DuplicateName.MaximumLength = ObjectName->MaximumLength;
    DuplicateName.Buffer = MmAllocatePoolWithTag( NonPagedPool, DuplicateName.MaximumLength, OB_TAG );
    OriginalDuplicate = DuplicateName.Buffer;
    RtlCopyMemory( DuplicateName.Buffer, ObjectName->Buffer, DuplicateName.MaximumLength );

    ntStatus = ObParseDirectory( &DuplicateName, &ObjectParsed );

    MmFreePoolWithTag( OriginalDuplicate, OB_TAG );

    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    //
    // TODO: Fix this. also impl a deletion queue
    //

    if ( Type != NULL && ObpGetHeaderFromObject( ObjectParsed )->Type != Type ) {

        return STATUS_INVALID_OBJECT_TYPE;
    }

    ObReferenceObject( ObjectParsed );
    *Object = ObjectParsed;
    return STATUS_SUCCESS;
}

VOID
ObQueryObjectList(
    _In_ POBJECT_TYPE     Type,
    _In_ PQUERY_PROCEDURE Procedure,
    _In_ PVOID            Context
)
{
    PLIST_ENTRY Flink;
    POBJECT_HEADER Header;
    KIRQL PreviousIrql;

    KeAcquireSpinLock( &Type->ObjectListLock, &PreviousIrql );
    Flink = Type->ObjectList;
    do {
        Header = CONTAINING_RECORD( Flink, OBJECT_HEADER, ObjectList );

        if ( !Procedure( Type, ObpGetObjectFromHeader( Header ), Context ) ) {

            KeReleaseSpinLock( &Type->ObjectListLock, PreviousIrql );
            return;
        }

        Flink = Flink->Flink;
    } while ( Flink != Type->ObjectList );
    KeReleaseSpinLock( &Type->ObjectListLock, PreviousIrql );
    return;
}
