


#define NTUSER_INTERNAL
#include <carbsup.h>
#include "usersup.h"
#include "ntuser.h"

PLIST_ENTRY ClassList = NULL;
KSPIN_LOCK  ClassListLock = { 0 };

NTSTATUS
NtFindClassByName(
    _Out_ PWND_CLASS* WindowClass,
    _In_  PWSTR       ClassName
)
{
    KIRQL PreviousIrql;
    PLIST_ENTRY Flink;
    PWND_CLASS WindowClassLink;

    KeAcquireSpinLock( &ClassListLock, &PreviousIrql );

    Flink = ClassList;
    do {
        WindowClassLink = CONTAINING_RECORD( Flink, WND_CLASS, ClassLinks );

        if ( RtlCompareString( ClassName, WindowClassLink->ClassName, FALSE ) == 0 ) {

            *WindowClass = WindowClassLink;
            KeReleaseSpinLock( &ClassListLock, PreviousIrql );
            return STATUS_SUCCESS;
        }

        Flink = Flink->Flink;
    } while ( Flink != ClassList );

    KeReleaseSpinLock( &ClassListLock, PreviousIrql );
    return STATUS_NOT_FOUND;
}

NTSTATUS
NtRegisterClass(
    _In_ PWND_CLASS Class
)
{
    PWND_CLASS NewClass;
    KIRQL PreviousIrql;

    //
    // Classes are not completely done, not sure how im going
    // to implement wnd_init and wnd_proc properly, i think i will
    // use user.dll as a gate
    //

    NewClass = MmAllocatePoolWithTag( NonPagedPool,
                                      sizeof( WND_CLASS ),
                                      USER_TAG );

    if ( PsGetPreviousMode( PsGetCurrentThread( ) ) == KernelMode ) {

        //
        // This is to catch any of the initial class setups
        //

        RtlCopyMemory( NewClass, Class, sizeof( WND_CLASS ) );
    }
    else {
        RtlCopyMemory( NewClass, Class, 152 ); // the size of the WND_CLASS inside usersup.h
        NewClass->DefWndProc = NtClassWindowBaseProc;
    }

    KeAcquireSpinLock( &ClassListLock, &PreviousIrql );
    if ( ClassList == NULL ) {

        KeInitializeHeadList( &NewClass->ClassLinks );
        ClassList = &NewClass->ClassLinks;
    }
    else {

        KeInsertTailList( ClassList, &NewClass->ClassLinks );
    }
    KeReleaseSpinLock( &ClassListLock, PreviousIrql );

    return STATUS_SUCCESS;
}

VOID
NtUnregisterClass(
    _In_ PWND_CLASS Class
)
{
    Class;

}

BOOLEAN
NtDefaultWindowProc(
    _In_ HANDLE  WindowHandle,
    _In_ ULONG32 MessageId,
    _In_ ULONG64 Param1,
    _In_ ULONG64 Param2
)
{
    NTSTATUS ntStatus;
    PKWND WindowObject;
    BOOLEAN Return;

    ntStatus = ObReferenceObjectByHandle( &WindowObject,
                                          WindowHandle,
                                          0,
                                          UserMode,
                                          NtWindowObject );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return FALSE;
    }

    if ( WindowObject->WindowClass->DefWndProc != NULL ) {

        Return = WindowObject->WindowClass->DefWndProc( WindowObject,
                                                        MessageId,
                                                        Param1,
                                                        Param2 );
    }
    else {
        Return = TRUE;
    }

    ObDereferenceObject( WindowObject );

    return Return;
}

NTSTATUS
NtGetWindowProc(
    _In_  HANDLE    WindowHandle,
    _Out_ WND_PROC* WndProc
)
{
    NTSTATUS ntStatus;
    PKWND WindowObject;

    ntStatus = ObReferenceObjectByHandle( &WindowObject,
                                          WindowHandle,
                                          0,
                                          UserMode,
                                          NtWindowObject );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    __try {

        *WndProc = WindowObject->WindowClass->WndProc;
    }
    __except ( EXCEPTION_EXECUTE_HANDLER ) {

        ObDereferenceObject( WindowObject );
        RtlRaiseException( STATUS_ACCESS_VIOLATION );
    }

    ObDereferenceObject( WindowObject );
    return STATUS_SUCCESS;
}
