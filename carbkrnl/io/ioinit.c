


#include <carbsup.h>
#include "iop.h"

VOID
IoInitializeIoManager(

)
{
    NTSTATUS ntStatus;
    HANDLE ThreadHandle;
    ULONG64 Queue;
    OBJECT_ATTRIBUTES ThreadAttributes = { { 0 }, { 0 }, OBJ_KERNEL_HANDLE };

    IopRequestQueueCount = KeQueryProcessorCount( );
    IopRequestQueueList = MmAllocatePoolWithTag( NonPagedPoolZeroed,
                                                 sizeof( IO_REQUEST_QUEUE ) * IopRequestQueueCount,
                                                 IO_TAG );

    for ( Queue = 0; Queue < IopRequestQueueCount; Queue++ ) {

        //
        // Create a thread pool worker on each processor
        //

        IopRequestQueueList[ Queue ].QueueId = Queue;
        KeInitializeEvent( &IopRequestQueueList[ Queue ].QueueEvent, FALSE );

        ntStatus = ZwCreateThread( &ThreadHandle,
                                   ZwCurrentProcess( ),
                                   THREAD_ALL_ACCESS,
                                   ( PKSTART_ROUTINE )IopDispatchQueue,
                                   &IopRequestQueueList[ Queue ],
                                   THREAD_SYSTEM | THREAD_SUSPENDED,
                                   &ThreadAttributes,
                                   0,
                                   NULL );
        if ( !NT_SUCCESS( ntStatus ) ) {

            KeBugCheck( STATUS_KERNEL_INITIALIZATION_FAILURE );
        }

        ntStatus = ObReferenceObjectByHandle( &IopRequestQueueList[ Queue ].QueueDispatcher,
                                              ThreadHandle,
                                              0,
                                              KernelMode,
                                              NULL );
        if ( !NT_SUCCESS( ntStatus ) ) {

            KeBugCheck( STATUS_KERNEL_INITIALIZATION_FAILURE );
        }

        ZwClose( ThreadHandle );

        PsSetThreadProcessor( IopRequestQueueList[ Queue ].QueueDispatcher, Queue );
        PsResumeThread( IopRequestQueueList[ Queue ].QueueDispatcher, NULL );
    }

}
