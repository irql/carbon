


#define NTUSER_INTERNAL
#include <carbsup.h>
#include "usersup.h"
#include "ntuser.h"

PKWND          RootWindow;
PKWND          FocusWindow;

POBJECT_TYPE   NtWindowObject;
POBJECT_TYPE   NtDeviceContext;

HANDLE         UpdateEvent;

PDC            Composed;
PKFONT         NtSystemFont;

VOID
NtUpdateDisplayThread(

);

VOID
NtInitializeUserWindows(

)
{
    HANDLE ThreadHandle;
    UNICODE_STRING WindowObjectName = RTL_CONSTANT_STRING( L"Window" );
    UNICODE_STRING DeviceContextName = RTL_CONSTANT_STRING( L"DeviceContext" );
    RECT ScreenRect = { 0, 0, 0, 0 };

    //
    // Need to add a cleanup to close event handle and clear queue
    //

    ObCreateObjectType( &NtWindowObject,
                        &WindowObjectName,
                        USER_TAG,
                        NULL );

    ObCreateObjectType( &NtDeviceContext,
                        &DeviceContextName,
                        USER_TAG,
                        NULL );

    NtDdiCreateDC( &NtScreenDC,
                   &ScreenRect );

    OBJECT_ATTRIBUTES Thread = { { 0 }, { 0 }, OBJ_KERNEL_HANDLE };
    OBJECT_ATTRIBUTES Event = { { 0 }, { 0 }, OBJ_KERNEL_HANDLE | OBJ_EXCLUSIVE_OBJECT };

    ZwCreateEvent( &UpdateEvent,
                   FALSE,
                   &Event );

    HANDLE FontHandle;
    NtCreateFont( &FontHandle,
                  L"IBMVGA.BIN",
                  8,
                  16,
                  0 );

    ObReferenceObjectByHandle( &NtSystemFont,
                               FontHandle,
                               0,
                               KernelMode,
                               NtFontObject );
    //ZwClose( FontHandle );

    ZwCreateThread( &ThreadHandle,
                    ZwCurrentProcess( ),
                    THREAD_ALL_ACCESS,
                    ( PKSTART_ROUTINE )NtUpdateDisplayThread,
                    NULL,
                    THREAD_SYSTEM,
                    &Thread,
                    0,
                    NULL );
    // ZwClose( ThreadHandle );

    WND_CLASS STATIC_Class;

    lstrcpyW( STATIC_Class.ClassName, L"STATIC" );
    STATIC_Class.DefWndProc = NtClassStaticBaseProc;
    STATIC_Class.WndProc = NULL;
    NtRegisterClass( &STATIC_Class );

    WND_CLASS BUTTON_Class;

    lstrcpyW( BUTTON_Class.ClassName, L"BUTTON" );
    BUTTON_Class.DefWndProc = NtClassButtonBaseProc;
    BUTTON_Class.WndProc = NULL;
    NtRegisterClass( &BUTTON_Class );

}

NTSTATUS
NtCreateWindow(
    _Out_    PHANDLE WindowHandle,
    _In_opt_ HANDLE  ParentHandle,
    _In_     PWSTR   Name,
    _In_     PWSTR   Class,
    _In_     ULONG32 x,
    _In_     ULONG32 y,
    _In_     ULONG32 w,
    _In_     ULONG32 h,
    _In_     ULONG32 MenuId
)
{
    NTSTATUS ntStatus;
    PKWND WindowObject;
    PKWND ParentObject;
    OBJECT_ATTRIBUTES WindowAttributes = { 0 };
    OBJECT_ATTRIBUTES EventAttributes = { { 0 }, { 0 }, OBJ_KERNEL_HANDLE };
    PKWND LastChild;
    KIRQL PreviousIrql;
    PWND_CLASS WindowClass;
    RECT Rect;

    ParentObject = NULL;
    WindowObject = NULL;

    ntStatus = NtFindClassByName( &WindowClass, Class );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    if ( ARGUMENT_PRESENT( ParentHandle ) ) {
        ntStatus = ObReferenceObjectByHandle( &ParentObject,
                                              ParentHandle,
                                              0,
                                              UserMode,
                                              NtWindowObject );
        if ( !NT_SUCCESS( ntStatus ) ) {

            goto NtProcedureFinished;
        }
    }

    ntStatus = ObCreateObject( &WindowObject,
                               NtWindowObject,
                               &WindowAttributes,
                               sizeof( KWND ) );
    if ( !NT_SUCCESS( ntStatus ) ) {

        goto NtProcedureFinished;
    }

    ntStatus = ObOpenObjectFromPointer( WindowHandle,
                                        WindowObject,
                                        0,
                                        0,
                                        UserMode );
    if ( !NT_SUCCESS( ntStatus ) ) {

        goto NtProcedureFinished;
    }

    ntStatus = ZwCreateEvent( &WindowObject->MessageEvent,
                              FALSE,
                              &EventAttributes );
    if ( !NT_SUCCESS( ntStatus ) ) {

        ZwClose( *WindowHandle );
        goto NtProcedureFinished;
    }

    WindowObject->WindowClass = WindowClass;
    Rect.Top = y;
    Rect.Left = x;
    Rect.Right = x + w;
    Rect.Bottom = y + h;
    WindowObject->WindowInfo.MenuId = MenuId;
    lstrcpyW( WindowObject->WindowInfo.Name, Name );

    NtDdiCreateDC( &WindowObject->BackContext,
                   &Rect );
    NtDdiCreateDC( &WindowObject->FrontContext,
                   &Rect );

    if ( ParentObject != NULL ) {
        WindowObject->Parent = ParentObject;
        WindowObject->Child = NULL;

        KeAcquireSpinLock( &ParentObject->LinkLock, &PreviousIrql );
        LastChild = ParentObject->Child;
        if ( LastChild == NULL ) {

            ParentObject->Child = WindowObject;
        }
        else {

            while ( LastChild->Child != NULL ) {

                LastChild = LastChild->Child;
            }
            LastChild->Child = WindowObject;
        }
        KeReleaseSpinLock( &ParentObject->LinkLock, PreviousIrql );
    }
    else {

        WindowObject->Child = NULL;
        WindowObject->Parent = WindowObject;
    }

    if ( RootWindow == NULL ) {

        RootWindow = WindowObject;
        RootWindow->Next = NULL;
    }
    else if ( ParentObject == NULL ) {

        KeAcquireSpinLock( &RootWindow->LinkLock, &PreviousIrql );

        WindowObject->Next = RootWindow->Next;
        RootWindow->Next = WindowObject;

        KeReleaseSpinLock( &RootWindow->LinkLock, PreviousIrql );
    }

    NtSendDirectMessage( WindowObject,
                         WM_ACTIVATE,
                         0,
                         0 );
    NtSendDirectMessage( WindowObject,
                         WM_PAINT,
                         0,
                         0 );

NtProcedureFinished:;

    if ( ParentObject != NULL ) {

        ObDereferenceObject( ParentObject );
    }

    if ( WindowObject != NULL ) {

        ObDereferenceObject( WindowObject );
    }

    return ntStatus;
}

PKUSER_MESSAGE
NtAllocateUserMessage(

)
{

    return MmAllocatePoolWithTag( NonPagedPool,
                                  sizeof( KUSER_MESSAGE ),
                                  USER_TAG );
}

VOID
NtFreeUserMessage(
    _In_ PKUSER_MESSAGE Message
)
{
    MmFreePoolWithTag( Message, USER_TAG );
}

VOID
NtSendMessage(
    _In_ HANDLE  WindowHandle,
    _In_ ULONG32 MessageId,
    _In_ ULONG64 Param1,
    _In_ ULONG64 Param2
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

        // Shut the fuck up.
        return;
    }

    NtSendDirectMessage( WindowObject, MessageId, Param1, Param2 );
    ObDereferenceObject( WindowObject );
}

VOID
NtBroadcastDirectMessage(
    _In_ ULONG32 MessageId,
    _In_ ULONG64 Param1,
    _In_ ULONG64 Param2
)
{

    PKWND CurrentWindow;
    PKWND ChildWindow;
    KIRQL PreviousIrql;

    KeAcquireSpinLock( &RootWindow->LinkLock, &PreviousIrql );
    CurrentWindow = RootWindow->Next;

    while ( CurrentWindow != NULL ) {

        KeAcquireSpinLockAtDpcLevel( &CurrentWindow->LinkLock );

        ChildWindow = CurrentWindow;
        do {
            NtSendDirectMessage( ChildWindow,
                                 MessageId,
                                 Param1,
                                 Param2 );

            ChildWindow = ChildWindow->Child;
        } while ( ChildWindow != NULL );

        KeReleaseSpinLockAtDpcLevel( &CurrentWindow->LinkLock );
        CurrentWindow = CurrentWindow->Next;
    }
    KeReleaseSpinLock( &RootWindow->LinkLock, PreviousIrql );

}

VOID
NtSendDirectMessage(
    _In_ PKWND   WindowObject,
    _In_ ULONG32 MessageId,
    _In_ ULONG64 Param1,
    _In_ ULONG64 Param2
)
{
    KIRQL PreviousIrql;
    PKUSER_MESSAGE CurrentMessage;
    PKUSER_MESSAGE Message;

    Message = NtAllocateUserMessage( );

    Message->MessageQueue = NULL;
    Message->WindowObject = WindowObject;
    Message->MessageId = MessageId;
    Message->Param1 = Param1;
    Message->Param2 = Param2;

    KeAcquireSpinLock( &WindowObject->MessageQueueLock, &PreviousIrql );

    if ( WindowObject->MessageQueue == NULL ) {

        WindowObject->MessageQueue = Message;
    }
    else {

        CurrentMessage = WindowObject->MessageQueue;
        while ( CurrentMessage->MessageQueue != NULL ) {

            CurrentMessage = CurrentMessage->MessageQueue;
        }

        CurrentMessage->MessageQueue = Message;
    }

    KeReleaseSpinLock( &WindowObject->MessageQueueLock, PreviousIrql );

    ZwSignalEvent( WindowObject->MessageEvent, TRUE );

    if ( MessageId != WM_PAINT ) {
#if 0
        NtSendDirectMessage( WindowObject,
                             WM_PAINT,
                             0,
                             0 );
#endif
        NtBroadcastDirectMessage( WM_PAINT,
                                  0,
                                  0 );
        //ZwSignalEvent( UpdateEvent, TRUE );
    }

    // mhh, not sure about how refs should work with these
    // decided to just leave a reference for the message
    //ObDereferenceObject( WindowObject );
}

BOOLEAN
NtReceiveMessage(
    _In_  HANDLE         WindowHandle,
    _Out_ PKUSER_MESSAGE Message
)
{
    NTSTATUS ntStatus;
    PKWND WindowObject;
    KIRQL PreviousIrql;
    PKUSER_MESSAGE LastMessage;

    ntStatus = ObReferenceObjectByHandle( &WindowObject,
                                          WindowHandle,
                                          0,
                                          UserMode,
                                          NtWindowObject );
    if ( !NT_SUCCESS( ntStatus ) ) {

        // Shut the fuck up.
        return FALSE;
    }

    //ntStatus = ZwWaitForSingleObject( WindowObject->MessageEvent,
    //                                  100 );

    //if ( ntStatus == STATUS_WAIT_INCOMPLETE ) {

    BOOLEAN Event;

    ZwQueryEvent( WindowObject->MessageEvent, &Event );

    if ( Event &&
         WindowObject->MessageQueue != NULL ) {

        KeAcquireSpinLock( &WindowObject->MessageQueueLock, &PreviousIrql );

        __try {

            Message->MessageId = WindowObject->MessageQueue->MessageId;
            Message->Param1 = WindowObject->MessageQueue->Param1;
            Message->Param2 = WindowObject->MessageQueue->Param2;
        }
        __except ( EXCEPTION_EXECUTE_HANDLER ) {

            KeReleaseSpinLock( &WindowObject->MessageQueueLock, PreviousIrql );
            ObDereferenceObject( WindowObject );
            RtlRaiseException( STATUS_ACCESS_VIOLATION );
        }

        LastMessage = WindowObject->MessageQueue;
        WindowObject->MessageQueue = LastMessage->MessageQueue;

        NtFreeUserMessage( LastMessage );

        KeReleaseSpinLock( &WindowObject->MessageQueueLock, PreviousIrql );

        if ( WindowObject->MessageQueue == NULL ) {

            ZwSignalEvent( WindowObject->MessageEvent, FALSE );
        }

    }

    ObDereferenceObject( WindowObject );
    return Event;
}

VOID
NtWaitMessage(
    _In_ HANDLE WindowHandle
)
{
    //
    // This & NtRecvMessage need improvement,
    // a better system for message's should be implemented.
    //

    NTSTATUS ntStatus;
    PKWND WindowObject;
    PKWND CurrentWindow;
    BOOLEAN Event;

    ntStatus = ObReferenceObjectByHandle( &WindowObject,
                                          WindowHandle,
                                          0,
                                          UserMode,
                                          NtWindowObject );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return;
    }

    //
    // Waits for any child event object to signal.
    //

    do {
        CurrentWindow = WindowObject->Parent;

        do {
            ZwQueryEvent( CurrentWindow->MessageEvent, &Event );
            if ( Event ) {

                return;
            }

            CurrentWindow = CurrentWindow->Child;
        } while ( CurrentWindow != NULL );

        ZwWaitForSingleObject( WindowObject->Parent->MessageEvent, 100 );
    } while ( TRUE );


}

VOID
NtGetWindowInfo(
    _In_  HANDLE    WindowHandle,
    _Out_ PWND_INFO WindowInfo
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

        return;
    }

    __try {

        RtlCopyMemory( WindowInfo, &WindowObject->WindowInfo, sizeof( WND_INFO ) );
        RtlCopyMemory( &WindowInfo->Rect, &WindowObject->BackContext->ClientArea, sizeof( RECT ) );
    }
    __except ( EXCEPTION_EXECUTE_HANDLER ) {

        ObDereferenceObject( WindowObject );
        RtlRaiseException( STATUS_ACCESS_VIOLATION );
    }

    ObDereferenceObject( WindowObject );
}

VOID
NtSetWindowInfo(
    _In_ HANDLE    WindowHandle,
    _In_ PWND_INFO WindowInfo
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

        return;
    }

    __try {

        RtlCopyMemory( &WindowObject->WindowInfo, WindowInfo, sizeof( WND_INFO ) );
    }
    __except ( EXCEPTION_EXECUTE_HANDLER ) {

        ObDereferenceObject( WindowObject );
        RtlRaiseException( STATUS_ACCESS_VIOLATION );
    }

    ObDereferenceObject( WindowObject );
}

VOID
NtUpdateDisplayThread(

)
{
    PKWND CurrentWindow;
    PKWND ChildWindow;
    KIRQL PreviousIrql;
    ULONG32 CurrentX;
    ULONG32 CurrentY;

    NtDdiCreateDC( &Composed,
                   &NtScreenDC->ClientArea );

    while ( TRUE ) {

        ZwWaitForSingleObject( UpdateEvent, WAIT_TIMEOUT_INFINITE );

        KeAcquireSpinLock( &RootWindow->LinkLock, &PreviousIrql );
        CurrentWindow = RootWindow;

        while ( CurrentWindow != NULL ) {

            // uncomment for kernel mode windows with no message thread

            if ( CurrentWindow != RootWindow ) {
                /*
                CurrentWindow->WindowClass->DefWndProc( CurrentWindow,
                                                        WM_PAINT,
                                                        0,
                                                        0 );*/

                KeAcquireSpinLockAtDpcLevel( &CurrentWindow->LinkLock );
            }
            else {

                CurrentWindow->WindowClass->DefWndProc( CurrentWindow,
                                                        WM_PAINT,
                                                        0,
                                                        0 );
                ZwSignalEvent( UpdateEvent, FALSE ); // this will be signalled by the WM_PAINT
            }

            NtDdiBlt( CurrentWindow->FrontContext,
                      0,
                      0,
                      CurrentWindow->FrontContext->ClientArea.Right -
                      CurrentWindow->FrontContext->ClientArea.Left,
                      CurrentWindow->FrontContext->ClientArea.Bottom -
                      CurrentWindow->FrontContext->ClientArea.Top,
                      Composed,
                      CurrentWindow->FrontContext->ClientArea.Left,
                      CurrentWindow->FrontContext->ClientArea.Top );

            ChildWindow = CurrentWindow->Child;
            while ( ChildWindow != NULL ) {

                NtDdiBlt( ChildWindow->FrontContext,
                          0,
                          0,
                          ChildWindow->FrontContext->ClientArea.Right -
                          ChildWindow->FrontContext->ClientArea.Left,
                          ChildWindow->FrontContext->ClientArea.Bottom -
                          ChildWindow->FrontContext->ClientArea.Top,
                          Composed,
                          CurrentWindow->FrontContext->ClientArea.Left +
                          ChildWindow->FrontContext->ClientArea.Left,
                          CurrentWindow->FrontContext->ClientArea.Top +
                          ChildWindow->FrontContext->ClientArea.Top );

                ChildWindow = ChildWindow->Child;
            }

            if ( CurrentWindow != RootWindow ) {

                KeReleaseSpinLockAtDpcLevel( &CurrentWindow->LinkLock );
            }

            CurrentWindow = CurrentWindow->Next;
        }
        KeReleaseSpinLock( &RootWindow->LinkLock, PreviousIrql );

        NtDdiBlt( Composed,
                  0,
                  0,
                  Composed->ClientArea.Right,
                  Composed->ClientArea.Bottom,
                  NtScreenDC,
                  0,
                  0 );

        NtGetCursorPosition( &CurrentX, &CurrentY );
        NtSetCursorPosition( CurrentX, CurrentY );
    }
}

PKWND
NtWindowFromPoint(
    _In_ ULONG32 x,
    _In_ ULONG32 y
)
{
    //
    // POGGERSSSSSSS! I ripped this all from the old carbon 
    // because shut the fuck up
    //

    PKWND CurrentWindow;
    PKWND LastWindow;
    PKWND PointWindow;
    PKWND ChildWindow;
    KIRQL PreviousIrql;

    CurrentWindow = RootWindow;
    KeAcquireSpinLock( &RootWindow->LinkLock, &PreviousIrql );

    LastWindow = CurrentWindow;
    while ( LastWindow->Next != NULL ) {

        LastWindow = LastWindow->Next;
    }

    while ( LastWindow != RootWindow ) {
        CurrentWindow = LastWindow;

        LastWindow = RootWindow;
        while ( LastWindow->Next != CurrentWindow ) {

            LastWindow = LastWindow->Next;
        }

        if ( CurrentWindow == RootWindow ) {

            continue;
        }

        if ( CurrentWindow->FrontContext->ClientArea.Top > ( LONG32 )y ||
             CurrentWindow->FrontContext->ClientArea.Bottom < ( LONG32 )y ) {

            continue;
        }

        if ( CurrentWindow->FrontContext->ClientArea.Left > ( LONG32 )x ||
             CurrentWindow->FrontContext->ClientArea.Right < ( LONG32 )x ) {

            continue;
        }

        //
        // if code got here, your pointer landed somewhere on 
        // a parent window, so we need to check each child windows
        // client area now.
        //

        PointWindow = CurrentWindow;
        if ( CurrentWindow->Child != NULL ) {

            LastWindow = CurrentWindow->Child;
            while ( LastWindow->Child != NULL ) {

                LastWindow = LastWindow->Child;
            }

            ChildWindow = CurrentWindow->Child;

            while ( LastWindow != CurrentWindow->Child ) {
                ChildWindow = LastWindow;

                LastWindow = CurrentWindow->Child;
                while ( LastWindow->Child != ChildWindow ) {

                    LastWindow = LastWindow->Child;
                }

                if ( CurrentWindow->FrontContext->ClientArea.Top + ChildWindow->FrontContext->ClientArea.Top > ( LONG32 )y ||
                     CurrentWindow->FrontContext->ClientArea.Top + ChildWindow->FrontContext->ClientArea.Bottom < ( LONG32 )y ) {

                    continue;
                }

                if ( CurrentWindow->FrontContext->ClientArea.Left + ChildWindow->FrontContext->ClientArea.Left > ( LONG32 )x ||
                     CurrentWindow->FrontContext->ClientArea.Left + ChildWindow->FrontContext->ClientArea.Right < ( LONG32 )x ) {

                    continue;
                }

                PointWindow = ChildWindow;
            }

            ChildWindow = CurrentWindow->Child;

            do {

                if ( CurrentWindow->FrontContext->ClientArea.Top + ChildWindow->FrontContext->ClientArea.Top > ( LONG32 )y ||
                     CurrentWindow->FrontContext->ClientArea.Top + ChildWindow->FrontContext->ClientArea.Bottom < ( LONG32 )y ) {

                    continue;
                }

                if ( CurrentWindow->FrontContext->ClientArea.Left + ChildWindow->FrontContext->ClientArea.Left > ( LONG32 )x ||
                     CurrentWindow->FrontContext->ClientArea.Left + ChildWindow->FrontContext->ClientArea.Right < ( LONG32 )x ) {

                    continue;
                }

                PointWindow = ChildWindow;
            } while ( FALSE );
        }

        KeReleaseSpinLock( &RootWindow->LinkLock, PreviousIrql );
        return PointWindow;
    }

    KeReleaseSpinLock( &RootWindow->LinkLock, PreviousIrql );
    return RootWindow;
}

VOID
NtRemoveWindow(
    _In_ PKWND Remove
)
{
    PKWND CurrentWindow;
    CurrentWindow = RootWindow;

    while ( CurrentWindow->Next != Remove ) {

        CurrentWindow = CurrentWindow->Next;
    }

    CurrentWindow->Next = Remove->Next;
    Remove->Next = NULL;
}

VOID
NtInsertWindow(
    _In_ PKWND Insert
)
{
    PKWND CurrentWindow;
    CurrentWindow = RootWindow;

    while ( CurrentWindow->Next != NULL ) {

        CurrentWindow = CurrentWindow->Next;
    }

    CurrentWindow->Next = Insert;
    Insert->Next = NULL;
}
