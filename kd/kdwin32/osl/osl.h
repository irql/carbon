


#pragma once

EXTERN_C
PVOID
OslAllocate(
    _In_ ULONG64 Length
);

EXTERN_C
VOID
OslFree(
    _In_ PVOID Address
);

EXTERN_C
VOID
OslWriteConsole(
    __in PCWCHAR Buffer,
    __in ...
);

EXTERN_C
ULONG
OslReadConsole(
    _In_ PWSTR Buffer,
    _In_ ULONG Length
);

EXTERN_C
VOID
OslSignalBrokeIn(

);

EXTERN_C
VOID
OslSignalContinue(

);

EXTERN_C
VOID
OslWaitForBreakIn(

);

EXTERN_C
VOID
OslAcquireCommunicationLock(

);

EXTERN_C
VOID
OslReleaseCommunicationLock(

);

EXTERN_C
VOID
OslAcquireModuleCacheLock(

);

EXTERN_C
VOID
OslReleaseModuleCacheLock(

);

EXTERN_C
VOID
OslDelayExecution(
    _In_ ULONG Milliseconds
);

EXTERN_C
VOID
KdpSendString(
    _In_ PVOID   Buffer,
    _In_ ULONG32 Length
);

EXTERN_C
KD_STATUS
OslSendPacket(
    _In_ KD_PACKET_TYPE PacketType,
    _In_ PVOID          Buffer,
    _In_ ULONG32        Length
);

EXTERN_C
KD_STATUS
OslReceivePacket(
    _In_ KD_PACKET_TYPE PacketType,
    _In_ PVOID          Buffer,
    _In_ ULONG32        Length
);
