


#define NTUSER_INTERNAL
#include <carbsup.h>
#include "usersup.h"
#include "ntuser.h"

#ifdef OLD_SYS_MSG
PKUSER_MESSAGE SystemMessageQueue = NULL;
KSPIN_LOCK     SystemMessageQueueLock = { 0 };
HANDLE         SystemMessageEvent;
#else
IO_QUEUE       SystemMessageQueue;
KEVENT         SystemMessageEvent;
#endif

VOID
NtSystemMessageThread(

);

VOID
NtInitializeSystemMessages(

)
{
    HANDLE ThreadHandle;
    OBJECT_ATTRIBUTES Thread = { { 0 }, { 0 }, OBJ_KERNEL_HANDLE };

    IoQueueCreate( &SystemMessageQueue, 128, sizeof( KUSER_MESSAGE ) );

    KeInitializeEvent( &SystemMessageEvent, FALSE );

    ZwCreateThread( &ThreadHandle,
                    ZwCurrentProcess( ),
                    THREAD_ALL_ACCESS,
                    ( PKSTART_ROUTINE )NtSystemMessageThread,
                    NULL,
                    THREAD_SYSTEM,
                    &Thread,
                    0,
                    NULL );
    //ZwClose( ThreadHandle );
}

#ifdef OLD_SYS_MSG
PKUSER_MESSAGE
NtDequeueSystemMessage(

)
{
    PKUSER_MESSAGE Last;
    KIRQL PreviousIrql;

    KeAcquireSpinLock( &SystemMessageQueueLock, &PreviousIrql );

    Last = SystemMessageQueue;
    SystemMessageQueue = SystemMessageQueue->MessageQueue;

    KeReleaseSpinLock( &SystemMessageQueueLock, PreviousIrql );

    if ( SystemMessageQueue == NULL ) {

        // should already be false, but shut up
        ZwSignalEvent( SystemMessageEvent, FALSE );
    }

    return Last;
}

VOID
NtEnqueueSystemMessage(
    _In_ PKUSER_MESSAGE Message
)
{
    PKUSER_MESSAGE CurrentMessage;
    KIRQL PreviousIrql;

    KeAcquireSpinLock( &SystemMessageQueueLock, &PreviousIrql );

    Message->MessageQueue = NULL;

    if ( SystemMessageQueue == NULL ) {

        SystemMessageQueue = Message;
    }
    else {

        CurrentMessage = SystemMessageQueue;
        while ( CurrentMessage->MessageQueue != NULL ) {

            CurrentMessage = CurrentMessage->MessageQueue;
        }

        CurrentMessage->MessageQueue = Message;
    }

    KeReleaseSpinLock( &SystemMessageQueueLock, PreviousIrql );

    ZwSignalEvent( SystemMessageEvent, TRUE );
}
#endif

VOID
NtSendSystemMessage(
    _In_ ULONG32 MessageId,
    _In_ ULONG64 Param1,
    _In_ ULONG64 Param2
)
{
#ifdef OLD_SYS_MSG
    PKUSER_MESSAGE Message;

    Message = NtAllocateUserMessage( );
    Message->MessageId = MessageId;
    Message->Param1 = Param1;
    Message->Param2 = Param2;

    NtEnqueueSystemMessage( Message );
#endif

    KUSER_MESSAGE Message;
    Message.MessageId = MessageId;
    Message.Param1 = Param1;
    Message.Param2 = Param2;

    IoQueueEnqueue( &SystemMessageQueue, &Message );
    KeSignalEvent( &SystemMessageEvent, TRUE );
}

VOID
NtReceiveSystemMessage(
    _In_ PKUSER_MESSAGE Buffer
)
{
#ifdef OLD_SYS_MSG
    PKUSER_MESSAGE Message;

    ZwWaitForSingleObject( SystemMessageEvent, WAIT_TIMEOUT_INFINITE );

    Message = NtDequeueSystemMessage( );
    __try {

        Buffer->MessageId = Message->MessageId;
        Buffer->Param1 = Message->Param1;
        Buffer->Param2 = Message->Param2;
        NtFreeUserMessage( Message );
    }
    __except ( EXCEPTION_EXECUTE_HANDLER ) {

        // BITCH
        NtFreeUserMessage( Message );
        RtlRaiseException( STATUS_ACCESS_VIOLATION );
    }
#endif

    KeWaitForSingleObject( &SystemMessageEvent, WAIT_TIMEOUT_INFINITE );

    NT_ASSERT( IoQueueDequeue( &SystemMessageQueue, Buffer ) );

    if ( SystemMessageQueue.QueueHead == SystemMessageQueue.QueueTail ) {

        KeSignalEvent( &SystemMessageEvent, FALSE );
    }
}

VOID
NtSystemMessageThread(

)
{
    // jon is bussin
    KUSER_MESSAGE SystemMessage;
    KIRQL PreviousIrql;

    STATIC ULONG32 PressX = 0;
    STATIC ULONG32 PressY = 0;
    STATIC BOOLEAN PressSet = FALSE;
    STATIC PKWND   PressWindow = NULL;

    ULONG32 PressXDist = 0;
    ULONG32 PressYDist = 0;

    while ( TRUE ) {

        NtReceiveSystemMessage( &SystemMessage );

        switch ( SystemMessage.MessageId ) {
        case WM_LMOUSEDOWN:;

            //
            // make it just check against the class to see 
            // if it can move, or the flags
            //

            if ( !PressSet ) {
                PressWindow = NtWindowFromPoint( ( ULONG32 )SystemMessage.Param1, ( ULONG32 )SystemMessage.Param2 );

                if ( PressWindow == RootWindow ) {

                    FocusWindow = PressWindow;

                    ZwSignalEvent( UpdateEvent, TRUE );
                }
                else if ( FocusWindow != PressWindow ) {
                    // add move flag check

                    KeAcquireSpinLock( &RootWindow->LinkLock, &PreviousIrql );
                    FocusWindow = PressWindow->Parent;

                    NtRemoveWindow( PressWindow->Parent );
                    NtInsertWindow( PressWindow->Parent );
                    KeReleaseSpinLock( &RootWindow->LinkLock, PreviousIrql );

                    NtSendDirectMessage( PressWindow->Parent,
                                         WM_FOCUS,
                                         0,
                                         0 );

                    ZwSignalEvent( UpdateEvent, TRUE );
                }

                PressX = ( ULONG32 )SystemMessage.Param1;
                PressY = ( ULONG32 )SystemMessage.Param2;
                PressSet = TRUE;

                NtSendDirectMessage( PressWindow,
                                     WM_LMOUSEDOWN,
                                     PressX,
                                     PressY );
            }

            break;
        case WM_LMOUSEUP:;

            if ( PressSet ) {

                if ( FocusWindow != RootWindow &&
                     FocusWindow == PressWindow ) {
                    PressXDist = ( ULONG32 )SystemMessage.Param1 - PressX;
                    PressYDist = ( ULONG32 )SystemMessage.Param2 - PressY;

                    FocusWindow->FrontContext->ClientArea.Left += PressXDist;
                    FocusWindow->FrontContext->ClientArea.Right += PressXDist;
                    FocusWindow->FrontContext->ClientArea.Top += PressYDist;
                    FocusWindow->FrontContext->ClientArea.Bottom += PressYDist;

                    FocusWindow->BackContext->ClientArea = FocusWindow->FrontContext->ClientArea;

                    ZwSignalEvent( UpdateEvent, TRUE );
                }
                PressSet = FALSE;

                NtSendDirectMessage( PressWindow,
                                     WM_LMOUSEUP,
                                     SystemMessage.Param1,
                                     SystemMessage.Param2 );
            }
            break;
        case WM_RMOUSEDOWN:;
        case WM_RMOUSEUP:;
        case WM_KEYDOWN:;
        case WM_KEYUP:;
            if ( PressWindow != NULL ) {
                NtSendDirectMessage( PressWindow,
                                     SystemMessage.MessageId,
                                     SystemMessage.Param1,
                                     SystemMessage.Param2 );
            }
            break;
        default:

            break;
        }

    }
}
