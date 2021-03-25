


#pragma once

#define IO_TAG '  oI'

VOID
IoInitializeIoManager(

);

VOID
IopCleanupDevice(
    _In_ PDEVICE_OBJECT Device
);

NTSTATUS
IopInvalidRequest(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP           Request
);

PDRIVER_OBJECT
IopCreateDriver(
    _In_ PUNICODE_STRING DriverName
);

VOID
IopCleanupFile(
    _In_ PIO_FILE_OBJECT FileObject
);

typedef struct _IO_REQUEST_QUEUE {

    //
    // Io request queues are allocated at initialization
    // a number of threads are created with dedicated queues.
    // Each thread will wait on the QueueEvent, this event
    // will be signalled by the io manager when the queue is
    // not empty.
    //

    ULONG64     QueueId;
    ULONG64     QueueLength;
    PIRP        Queue;
    KSPIN_LOCK  QueueLock;
    PKTHREAD    QueueDispatcher;
    KEVENT      QueueEvent;

} IO_REQUEST_QUEUE, *PIO_REQUEST_QUEUE;

VOID
IopEnqueueRequest(
    _In_ PIRP Request
);

VOID
IopDispatchQueue(
    _In_ VOLATILE PIO_REQUEST_QUEUE Queue
);

EXTERN VOLATILE ULONG64           IopRequestQueueCount;
EXTERN VOLATILE PIO_REQUEST_QUEUE IopRequestQueueList;

typedef struct _IO_INTERRUPT {

    KSERVICE_ROUTINE ServiceRoutine;
    PVOID            ServiceContext;
    ULONG            Vector;
    KIRQL            Irql;
    BOOLEAN          Connected;

    //
    // Link vectors by their vector
    //

    PIO_INTERRUPT    Link;

} IO_INTERRUPT, *PIO_INTERRUPT;

VOID
IopCleanupInterrupt(
    _In_ PIO_INTERRUPT InterruptObject
);

PIO_INTERRUPT
IopGetServiceHandler(
    _In_ ULONG Vector
);
