


#include <carbsup.h>
#include "iop.h"
#include "../hal/halp.h"
#include "../ke/ki.h"

POBJECT_TYPE IoInterruptObject;

KSPIN_LOCK    IopServiceLock = { 0 };
PIO_INTERRUPT IopServiceHandlers[ 256 ] = { NULL };

NTSTATUS
IoConnectInterrupt(
    _Out_ PIO_INTERRUPT*     InterruptObject,
    _In_  KSERVICE_ROUTINE   ServiceRoutine,
    _In_  PVOID              ServiceContext,
    _In_  ULONG              Vector,
    _In_  KIRQL              Irql,
    _In_  POBJECT_ATTRIBUTES Interrupt
)
{
    NTSTATUS ntStatus;
    PIO_INTERRUPT LinkInterrupt;
    KIRQL PreviousIrql;

    ntStatus = ObCreateObject( InterruptObject,
                               IoInterruptObject,
                               Interrupt,
                               sizeof( IO_INTERRUPT ) );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    ( *InterruptObject )->ServiceRoutine = ServiceRoutine;
    ( *InterruptObject )->ServiceContext = ServiceContext;
    ( *InterruptObject )->Vector = Vector;
    ( *InterruptObject )->Irql = Irql;
    ( *InterruptObject )->Link = NULL;
    ( *InterruptObject )->Connected = TRUE;

    KeAcquireSpinLock( &IopServiceLock, &PreviousIrql );
    LinkInterrupt = IopServiceHandlers[ Vector ];

    if ( LinkInterrupt == NULL ) {

        IopServiceHandlers[ Vector ] = *InterruptObject;
    }
    else {

        while ( LinkInterrupt->Link != NULL ) {

            LinkInterrupt = LinkInterrupt->Link;
        }

        LinkInterrupt->Link = *InterruptObject;
    }
    KeReleaseSpinLock( &IopServiceLock, PreviousIrql );

    //
    // Reference object for installment inside IopServiceHandlers
    //

    ObReferenceObject( *InterruptObject );

    return STATUS_SUCCESS;
}

VOID
IopCleanupInterrupt(
    _In_ PIO_INTERRUPT InterruptObject
)
{

    if ( InterruptObject->Connected ) {

        //
        // to be honest, just dont write shit code and this wouldn't
        // need to be here.
        //

        IoDisconnectInterrupt( InterruptObject );
    }
}

VOID
IoDisconnectInterrupt(
    _In_ PIO_INTERRUPT InterruptObject
)
{
    PIO_INTERRUPT LinkInterrupt;

    LinkInterrupt = IopServiceHandlers[ InterruptObject->Vector ];

    if ( LinkInterrupt == InterruptObject ) {

        IopServiceHandlers[ InterruptObject->Vector ] = InterruptObject->Link;
    }
    else {

        while ( LinkInterrupt->Link != InterruptObject ) {

            LinkInterrupt = LinkInterrupt->Link;
        }

        LinkInterrupt->Link = InterruptObject->Link;
    }

    InterruptObject->Connected = FALSE;
}

PIO_INTERRUPT
IopGetServiceHandler(
    _In_ ULONG Vector
)
{
    return IopServiceHandlers[ Vector ];
}

VOID
IoAcquireInterruptSafeLock(
    _In_ PKSPIN_LOCK SpinLock
)
{
    PIO_LOCK_CONTEXT LockContext;

    LockContext = &KeQueryCurrentProcessor( )->LockContext;

    //
    // one lock at a time you bitch
    //

    NT_ASSERT( LockContext->IsHolding == FALSE );

    LockContext->Lock = SpinLock;
    LockContext->PreviousIF = __readeflags( ) >> 9;
    LockContext->PreviousIF &= 1;

    _disable( );

    KeRaiseIrql( IPI_LEVEL, &LockContext->PreviousIrql );

    //KeAcquireSpinLockAtDpcLevel( LockContext->Lock );
    while ( _InterlockedCompareExchange64( ( volatile long long* )LockContext->Lock, 1, 0 ) != 0 )
        ;

    LockContext->IsHolding = TRUE;
}

VOID
IoReleaseInterruptSafeLock(
    _In_ PKSPIN_LOCK SpinLock
)
{
    PIO_LOCK_CONTEXT LockContext;

    LockContext = &KeQueryCurrentProcessor( )->LockContext;

    NT_ASSERT( LockContext->IsHolding == TRUE );
    NT_ASSERT( LockContext->Lock == SpinLock );

    //KeReleaseSpinLockAtDpcLevel( LockContext->Lock );
    *LockContext->Lock = 0;

    LockContext->IsHolding = FALSE;

    KeLowerIrql( LockContext->PreviousIrql );

    if ( LockContext->PreviousIF ) {

        _enable( );
    }
}
