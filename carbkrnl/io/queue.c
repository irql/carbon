


#include <carbsup.h>
#include "iop.h"

VOLATILE ULONG64           IopRequestQueueCount;
VOLATILE PIO_REQUEST_QUEUE IopRequestQueueList;

//
// ISSUE: address spaces for interrupt requests, the 
// system buffers need to be dealt with
//

VOID
IopEnqueueRequest(
    _In_ PIRP Request
)
{
    ULONG64 Queue;
    ULONG64 QueueLengthLow;
    PIO_REQUEST_QUEUE RequestQueue;
    KIRQL PreviousIrql;
    PIRP LastRequest;

    RequestQueue = IopRequestQueueList;
    QueueLengthLow = ~0ull;
    for ( Queue = 0; Queue < IopRequestQueueCount; Queue++ ) {
        if ( QueueLengthLow > RequestQueue[ Queue ].QueueLength ) {

            QueueLengthLow = RequestQueue[ Queue ].QueueLength;
            RequestQueue = &RequestQueue[ Queue ];
        }
    }

    RtlDebugPrint( L"requesting queue %d %ull\n", RequestQueue->QueueId, RequestQueue );
    KeAcquireSpinLock( &RequestQueue->QueueLock, &PreviousIrql );
    LastRequest = RequestQueue->Queue;
    if ( LastRequest == NULL ) {

        RequestQueue->Queue = Request;
        Request->NextQueueIrp = NULL;
    }
    else {

        while ( LastRequest->NextQueueIrp != NULL ) {
            LastRequest = LastRequest->NextQueueIrp;
        }
        LastRequest->NextQueueIrp = Request;
        Request->NextQueueIrp = NULL;
    }
    RequestQueue->QueueLength++;
    KeReleaseSpinLock( &RequestQueue->QueueLock, PreviousIrql );
    KeSignalEvent( &RequestQueue->QueueEvent, TRUE );

}

VOID
IopDispatchQueue(
    _In_ VOLATILE PIO_REQUEST_QUEUE Queue
)
{
    //RtlDebugPrint( L"IopDispatchQueue: %ull (%d) %d\n", Queue, Queue->QueueId, PsGetThreadId( PsGetCurrentThread( ) ) );
    NTSTATUS RequestStatus;
    KIRQL PreviousIrql;
    PIRP CurrentRequest;

    while ( TRUE ) {

        //
        // Wait for the event to be signalled, and then call the requested driver
        // with the request packet, the event should be signalled appropriately after removing
        // a request from the queue.
        //

        KeWaitForSingleObject( &Queue->QueueEvent, WAIT_TIMEOUT_INFINITE );
        RtlDebugPrint( L"request recieved on queue: %d %ull %ull %d!\n", Queue->QueueId, Queue, Queue->Queue, Queue->QueueEvent.Signaled );
        KeAcquireSpinLock( &Queue->QueueLock, &PreviousIrql );
        CurrentRequest = Queue->Queue;
        Queue->Queue = CurrentRequest->NextQueueIrp;

        if ( Queue->Queue == NULL ) {

            KeSignalEvent( &Queue->QueueEvent, FALSE );
        }

        KeReleaseSpinLock( &Queue->QueueLock, PreviousIrql );

        RequestStatus = IoCallDriver( CurrentRequest->DeviceObject, CurrentRequest );

        //
        // Not decided what to do when an asynchronous io operation 
        // fails directly from the driver.
        //

        ObDereferenceObject( CurrentRequest->Process );
        ObDereferenceObject( CurrentRequest->Thread );

        ObDereferenceObject( CurrentRequest->FileObject );
        IoFreeIrp( CurrentRequest );
    }
}

VOID
IoQueueCreate(
    _Out_ PIO_QUEUE Queue,
    _In_  ULONG64   QueueLength,
    _In_  ULONG64   PacketLength
)
{
    //
    // haha yes Length * PacketLength to make sure they're divisible
    //

    Queue->QueueBase = ( ULONG64 )MmAllocatePoolWithTag( NonPagedPool, QueueLength * PacketLength, IO_TAG );
    Queue->QueueLength = QueueLength;
    Queue->QueueHead = 0;
    Queue->QueueTail = 0;
    Queue->PacketLength = PacketLength;
    Queue->QueueLock = 0;
    Queue->QueueFree = 0;
}

BOOLEAN
IoQueueEnqueue(
    _In_ PIO_QUEUE Queue,
    _In_ PVOID     Packet
)
{
    if ( Queue->QueueFree ) {

        return FALSE;
    }

    IoAcquireInterruptSafeLock( &Queue->QueueLock );

    if ( Queue->QueueHead + Queue->QueueTail >= Queue->QueueLength ) {

        if ( Queue->QueueHead != 0 ) {

            //
            // if the head is not at 0, move it to zero, instead of doing
            // some stupid circular stuff.
            //

            RtlMoveMemory(
                ( PVOID )( Queue->QueueBase ),
                ( PVOID )( Queue->QueueBase + Queue->QueueHead * Queue->PacketLength ),
                ( ULONG32 )( ( Queue->QueueTail - Queue->QueueHead ) * Queue->PacketLength ) );
            Queue->QueueTail -= Queue->QueueHead;
            Queue->QueueHead = 0;
        }
        else {

            //
            // Discard the packet, not enough room.
            //

            IoReleaseInterruptSafeLock( &Queue->QueueLock );
            return FALSE;
        }
    }

    RtlCopyMemory(
        ( PVOID )( Queue->QueueBase + Queue->QueueTail * Queue->PacketLength ),
        Packet,
        ( ULONG32 )( Queue->PacketLength ) );
    Queue->QueueTail++;

    IoReleaseInterruptSafeLock( &Queue->QueueLock );
    return TRUE;
}

BOOLEAN
IoQueueDequeue(
    _In_  PIO_QUEUE Queue,
    _Out_ PVOID     Packet
)
{
    if ( Queue->QueueFree ) {

        return FALSE;
    }

    IoAcquireInterruptSafeLock( &Queue->QueueLock );

    if ( Queue->QueueHead == Queue->QueueTail ) {

        IoReleaseInterruptSafeLock( &Queue->QueueLock );
        return FALSE;
    }

    RtlCopyMemory(
        Packet,
        ( PVOID )( Queue->QueueBase + Queue->QueueHead * Queue->PacketLength ),
        ( ULONG32 )Queue->PacketLength );
    Queue->QueueHead++;

    IoReleaseInterruptSafeLock( &Queue->QueueLock );
    return TRUE;
}

VOID
IoQueueFree(
    _In_ PIO_QUEUE Queue
)
{
    KIRQL PreviousIrql;

    Queue->QueueFree = TRUE;

    // brutal
    KeAcquireSpinLock( &Queue->QueueLock, &PreviousIrql );
    KeReleaseSpinLock( &Queue->QueueLock, PreviousIrql );

    MmFreePoolWithTag( ( PVOID )Queue->QueueBase, IO_TAG );
}
