/*++

Module ObjectName:

	spinlock.c

Abstract:

	Spinlocks.

--*/

#include <carbsup.h>
#include "hal.h"
#include "ki.h"
#include "acpi.h"

/*
	the implementation should be changed to work with the dispatcher.
*/

ULONG64 KiDispatcherSpinlocks = 0;

VOID
KeAcquireSpinLock(
	__in PKSPIN_LOCK SpinLock
)
{
	if ( KiDispatcherSpinlocks ) {

		if ( SpinLock->ThreadLocked ) {

			KiSpinlockWaitThread( SpinLock );
		}

		return;
	}
	else {

		while ( _InterlockedCompareExchange64( ( volatile long long* )&SpinLock->ThreadLocked, 1, 0 ) == 1 )
			;
	}

	return;
}

VOID
KeReleaseSpinLock(
	__in PKSPIN_LOCK SpinLock
)
{

	//SpinLock->ThreadLocked = ( PKTHREAD )0;
	_InterlockedExchange64( ( volatile long long* )&SpinLock->ThreadLocked, 0 );

	return;
}


