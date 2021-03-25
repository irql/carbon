


#include <carbsup.h>
#include "../hal/halp.h"
#include "ki.h"
#include "../mm/mi.h"
#include "../ps/psp.h"

POBJECT_TYPE KeMutexObject;

VOID
KeInitializeMutex(
    _Inout_ PKMUTEX Mutex
)
{
    Mutex->Header.Type = DPC_OBJECT_MUTEX;
}

BOOLEAN
KeQueryMutex(
    _Inout_ PKMUTEX Mutex
)
{

    return ( BOOLEAN )!!Mutex->Owner;
}

BOOLEAN
KeTryAcquireMutex(
    _Inout_ PKMUTEX Mutex
)
{

    return _InterlockedCompareExchange64( ( volatile long long* )&Mutex->Owner, ( long long )PsGetCurrentThread( ), 0 ) == 0;
}

VOID
KeAcquireMutex(
    _Inout_ PKMUTEX Mutex
)
{

    if ( KeTryAcquireMutex( Mutex ) ) {

        return;
    }

    KeWaitForSingleObject( Mutex, WAIT_TIMEOUT_INFINITE );
}

VOID
KeReleaseMutex(
    _Inout_ PKMUTEX Mutex
)
{
    Mutex->Owner = NULL;
    KiEnsureAllProcessorsReady( );
}

