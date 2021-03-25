


#pragma once

NTSYSAPI EXTERN BOOLEAN KdDebuggerEnabled;

#ifndef KRNLINTERNAL

typedef struct _KPCB {
    PVOID           Reserved;
    ULONG64         ProcessorNumber;
    ULONG64         ApicId;
    BOOLEAN         InService;
} KPCB, *PKPCB;

#endif

typedef struct _KPCB_HEAD {
    PVOID           Reserved;
    ULONG64         ProcessorNumber;
    ULONG64         ApicId;
    BOOLEAN         InService;
} KPCB_HEAD, *PKPCB_HEAD;

FORCEINLINE
PKPCB
KeQueryCurrentProcessor(

)
{
    // MSR_KERNEL_GS_BASE

    return ( PKPCB )__readmsr( 0xC0000102 );//_readgsbase_u64( );//__readgsqword( 0 );
}

NTSYSAPI
PKPCB
KeQueryProcessorByNumber(
    _In_ ULONG64 ProcessorNumber
);

NTSYSAPI
ULONG64
KeQueryProcessorCount(

);

NTSYSAPI
VOID
KeRaiseIrql(
    _In_  KIRQL  NewIrql,
    _Out_ PKIRQL OldIrql
);

NTSYSAPI
VOID
KeLowerIrql(
    _In_ KIRQL NewIrql
);

NTSYSAPI
KIRQL
KeGetCurrentIrql(

);

typedef VOLATILE ULONG64 KSPIN_LOCK, *PKSPIN_LOCK;

FORCEINLINE
VOID
KeAcquireSpinLock(
    _In_  PKSPIN_LOCK SpinLock,
    _Out_ PKIRQL      PreviousIrql
)
{
    if ( KeQueryCurrentProcessor( ) != NULL ) {

        NT_ASSERT( !( ( PKPCB_HEAD )KeQueryCurrentProcessor( ) )->InService );
    }

    KeRaiseIrql( DISPATCH_LEVEL, PreviousIrql );

    while ( _InterlockedCompareExchange64( ( volatile long long* )SpinLock, 1, 0 ) != 0 )
        ;// _mm_pause( );
}

FORCEINLINE
VOID
KeAcquireSpinLockAtDpcLevel(
    _In_ PKSPIN_LOCK SpinLock
)
{
    NT_ASSERT( KeGetCurrentIrql( ) >= DISPATCH_LEVEL );

    if ( KeQueryCurrentProcessor( ) != NULL ) {

        NT_ASSERT( !( ( PKPCB_HEAD )KeQueryCurrentProcessor( ) )->InService );
    }

    while ( _InterlockedCompareExchange64( ( volatile long long* )SpinLock, 1, 0 ) != 0 )
        ;// _mm_pause( );
}

FORCEINLINE
VOID
KeReleaseSpinLock(
    _In_ PKSPIN_LOCK SpinLock,
    _In_ KIRQL       PreviousIrql
)
{
    *SpinLock = 0;
    KeLowerIrql( PreviousIrql );
}

FORCEINLINE
VOID
KeReleaseSpinLockAtDpcLevel(
    _In_ PKSPIN_LOCK SpinLock
)
{
    *SpinLock = 0;
}

FORCEINLINE
BOOLEAN
KeQuerySpinLock(
    _In_ PKSPIN_LOCK SpinLock
)
{
    return ( BOOLEAN )!!( *SpinLock );
}

NTSYSAPI
VOID
KeInitializeDpc(
    _In_ PKDPC              Dpc,
    _In_ PKDEFERRED_ROUTINE DeferredRoutine,
    _In_ PVOID              DeferredContext,
    _In_ KPRIORITY          Priority
);

NTSYSAPI
VOID
KeSetTargetProcessorDpc(
    _In_ PKDPC   Dpc,
    _In_ ULONG64 Number
);

NTSYSAPI
VOID
KeInsertQueueDpc(
    _In_ PKDPC Dpc
);

NTSYSAPI
VOID
KeGenericCallDpc(
    _In_ PKDEFERRED_ROUTINE DeferredRoutine,
    _In_ PVOID              DeferredContext
);

NTSYSAPI
VOID
KeInitializeEvent(
    _In_ PKEVENT Event,
    _In_ BOOLEAN Signal
);

NTSYSAPI
VOID
KeSignalEvent(
    _In_ PKEVENT Event,
    _In_ BOOLEAN Signal
);

NTSYSAPI
BOOLEAN
KeQueryEvent(
    _In_ PKEVENT Event
);

NTSYSAPI
VOID
KeWaitForSingleObject(
    _In_ PVOID   Object,
    _In_ ULONG64 Timeout
);

NTSYSAPI EXTERN POBJECT_TYPE KeEventObject;
NTSYSAPI EXTERN POBJECT_TYPE KeMutexObject;

NTSYSAPI
NORETURN
VOID
KeBugCheckEx(
    _In_ NTSTATUS Status,
    _In_ ULONG64  Code1,
    _In_ ULONG64  Code2,
    _In_ ULONG64  Code3,
    _In_ ULONG64  Code4
);

NTSYSAPI
NORETURN
VOID
KeBugCheck(
    _In_ NTSTATUS Status
);

NTSYSAPI
VOID
KeInitializeMutex(
    _Inout_ PKMUTEX Mutex
);

NTSYSAPI
BOOLEAN
KeQueryMutex(
    _Inout_ PKMUTEX Mutex
);

NTSYSAPI
BOOLEAN
KeTryAcquireMutex(
    _Inout_ PKMUTEX Mutex
);

NTSYSAPI
VOID
KeAcquireMutex(
    _Inout_ PKMUTEX Mutex
);

NTSYSAPI
VOID
KeReleaseMutex(
    _Inout_ PKMUTEX Mutex
);

#pragma pack(push, 1)

typedef struct _KSYSTEM_SERVICE {
    PVOID   Procedure;
    ULONG32 ArgumentCount;
    ULONG32 Alignment;
} KSYSTEM_SERVICE, *PKSYSTEM_SERVICE;

typedef struct _KSSDT {
    ULONG32          ServiceCount;
    ULONG32          Alignment;
    PKSYSTEM_SERVICE ServiceTable;
} KSSDT, *PKSSDT;

C_ASSERT( FIELD_OFFSET( KSSDT, ServiceCount ) == 0 );
C_ASSERT( FIELD_OFFSET( KSSDT, ServiceTable ) == 8 );
C_ASSERT( FIELD_OFFSET( KSYSTEM_SERVICE, Procedure ) == 0 );
C_ASSERT( FIELD_OFFSET( KSYSTEM_SERVICE, ArgumentCount ) == 8 );

#define SYSTEM_SERVICE( procedure, arguments ) { procedure, arguments }

#pragma pack(pop)

NTSYSAPI
NTSTATUS
KeInstallServiceDescriptorTable(
    _In_ ULONG32          ServiceTableIndex,
    _In_ ULONG32          ServiceCount,
    _In_ PKSYSTEM_SERVICE ServiceTable
);

NTSYSAPI
NTSTATUS
ZwCreateEvent(
    _Out_ PHANDLE            EventHandle,
    _In_  BOOLEAN            EventSignal,
    _In_  POBJECT_ATTRIBUTES EventAttributes
);

NTSYSAPI
NTSTATUS
ZwQueryEvent(
    _In_  HANDLE   EventHandle,
    _Out_ PBOOLEAN EventSignal
);

NTSYSAPI
NTSTATUS
ZwSignalEvent(
    _In_ HANDLE  EventHandle,
    _In_ BOOLEAN EventSignal
);

NTSYSAPI
NTSTATUS
ZwWaitForSingleObject(
    _In_ HANDLE  ObjectHandle,
    _In_ ULONG64 TimeOut
);

NTSTATUS
NtWaitForSingleObject(
    _In_ HANDLE  ObjectHandle,
    _In_ ULONG64 TimeOut
);

typedef VOID( *PKIPI_CALL )(
    _In_ PVOID
    );

NTSYSAPI
VOID
KeGenericCallIpi(
    _In_ PKIPI_CALL BroadcastFunction,
    _In_ PVOID      BroadcastContext
);

NTSYSAPI
VOID
KeQuerySystemTime(
    _Out_ PKSYSTEM_TIME SystemTime
);

NTSTATUS
NtQuerySystemClock(
    _In_ PKSYSTEM_TIME ClockTime
);

NTSTATUS
NtCreateMutex(
    _Out_ PHANDLE MutexHandle,
    _In_  BOOLEAN InitialOwner
);

NTSTATUS
NtReleaseMutex(
    _In_ HANDLE MutexHandle
);
