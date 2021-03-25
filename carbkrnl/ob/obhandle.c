


#include <carbsup.h>
#include "obp.h"
#include "../hal/halp.h"
#include "../ke/ki.h"

NTSTATUS
ObQueryHandleAccess(
    _Out_ PACCESS_MASK Access,
    _In_  HANDLE       Handle
)
{
    PKPROCESS Process;
    PLIST_ENTRY Flink;
    PHANDLE_TABLE_ENTRY Entry;
    KIRQL PreviousIrql;

    if ( Handle == NtCurrentProcess( ) ) {

        *Access = FULL_ACCESS;
        return STATUS_SUCCESS;
    }

    if ( Handle == NtCurrentThread( ) ) {

        *Access = FULL_ACCESS;
        return STATUS_SUCCESS;
    }

    Process = PsGetCurrentProcess( );

    KeAcquireSpinLock( &Process->HandleTable.HandleLock, &PreviousIrql );
    Flink = Process->HandleTable.HandleLinks;
    do {
        Entry = CONTAINING_RECORD( Flink, HANDLE_TABLE_ENTRY, HandleLinks );

        if ( Entry->Handle == Handle ) {
            *Access = Entry->Access;
            KeReleaseSpinLock( &Process->HandleTable.HandleLock, PreviousIrql );
            return STATUS_SUCCESS;
        }

        Flink = Flink->Flink;
    } while ( Flink != Process->HandleTable.HandleLinks );
    KeReleaseSpinLock( &Process->HandleTable.HandleLock, PreviousIrql );

    return STATUS_INVALID_HANDLE;
}

NTSTATUS
ObOpenObjectFromPointer(
    _Out_ PHANDLE         ObjectHandle,
    _In_  PVOID           Object,
    _In_  ACCESS_MASK     Access,
    _In_  ULONG           HandleAttributes,
    _In_  KPROCESSOR_MODE AccessMode
)
{
    PKPROCESS Process;
    PHANDLE_TABLE_ENTRY NewHandle;
    KIRQL PreviousIrql;

    if ( AccessMode == UserMode ) {

        //
        // AccessMode is only used to know how it should interpret other arguments.
        //

        if ( ( HandleAttributes & OBJ_KERNEL_HANDLE ) == OBJ_KERNEL_HANDLE ) {

            return STATUS_ACCESS_DENIED;
        }
    }

    ObpIncrementHandleCount( Object );
    NewHandle = MmAllocatePoolWithTag( NonPagedPool, sizeof( HANDLE_TABLE_ENTRY ), OB_TAG );
    NewHandle->Object = Object;

    if ( ( HandleAttributes & OBJ_KERNEL_HANDLE ) == OBJ_KERNEL_HANDLE ) {

        //
        // Negative ids for kernel handles
        //

        NewHandle->Handle = -( HANDLE )KeGenerateUniqueId( );
    }
    else {

        NewHandle->Handle = KeGenerateUniqueId( );
    }

    NewHandle->Access = Access;

    if ( ( HandleAttributes & OBJ_KERNEL_HANDLE ) == OBJ_KERNEL_HANDLE ) {

        //
        // Kernel handles always go in the system process.
        //

        Process = PsInitialSystemProcess;
    }
    else {

        Process = PsGetCurrentProcess( );
    }

    KeAcquireSpinLock( &Process->HandleTable.HandleLock, &PreviousIrql );
    if ( Process->HandleTable.HandleCount == 0 ) {
        KeInitializeListHead( &NewHandle->HandleLinks );
        Process->HandleTable.HandleLinks = &NewHandle->HandleLinks;
    }
    else {
        KeInsertEntryTail( Process->HandleTable.HandleLinks, &NewHandle->HandleLinks );
    }
    Process->HandleTable.HandleCount++;
    KeReleaseSpinLock( &Process->HandleTable.HandleLock, PreviousIrql );

    *ObjectHandle = NewHandle->Handle;
    return STATUS_SUCCESS;
}

NTSTATUS
ObCloseHandle(
    _In_ HANDLE Handle
)
{
    PVOID Object;
    PKPROCESS Process;
    PLIST_ENTRY Flink;
    POBJECT_HEADER Header;
    PHANDLE_TABLE_ENTRY Entry;
    KIRQL PreviousIrql;

    if ( Handle == NtCurrentProcess( ) ) {

        ObpDecrementHandleCount( PsGetCurrentProcess( ) );
        return STATUS_SUCCESS;
    }

    if ( Handle == NtCurrentThread( ) ) {

        ObpDecrementHandleCount( PsGetCurrentThread( ) );
        return STATUS_SUCCESS;
    }

    Process = PsGetCurrentProcess( );

    KeAcquireSpinLock( &Process->HandleTable.HandleLock, &PreviousIrql );
    Flink = Process->HandleTable.HandleLinks;
    do {
        Entry = CONTAINING_RECORD( Flink, HANDLE_TABLE_ENTRY, HandleLinks );

        if ( Entry->Handle == Handle ) {

            Object = Entry->Object;

            Process->HandleTable.HandleCount--;
            if ( Process->HandleTable.HandleCount == 0 ) {
                Process->HandleTable.HandleLinks = NULL;
            }
            else {
                KeRemoveListEntry( &Entry->HandleLinks );
            }
            MmFreePoolWithTag( Entry, OB_TAG );

            Header = ObpGetHeaderFromObject( Object );

            ObpDecrementHandleCount( Object );

            if ( Header->PointerCount == 0 &&
                 Header->HandleCount == 0 &&
                 ( Header->Flags & OBJ_PERMANENT_OBJECT ) == 0 ) {

                ObDestroyObject( Object );
            }

            KeReleaseSpinLock( &Process->HandleTable.HandleLock, PreviousIrql );
            return STATUS_SUCCESS;
        }

        Flink = Flink->Flink;
    } while ( Flink != Process->HandleTable.HandleLinks );
    KeReleaseSpinLock( &Process->HandleTable.HandleLock, PreviousIrql );

    return STATUS_INVALID_HANDLE;
}

NTSTATUS
ZwClose(
    _In_ HANDLE Handle
)
{
    return ObCloseHandle( Handle );
}

NTSTATUS
NtClose(
    _In_ HANDLE Handle
)
{
    return ObCloseHandle( Handle );
}
