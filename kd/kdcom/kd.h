


#pragma once

#ifdef __KD_COM__
#define KDAPI __declspec(dllexport)
#else
#define KDAPI __declspec(dllimport)
#endif

KDAPI
VOID
KdDispatchException(
    _In_ PEXCEPTION_RECORD TrapFrame
);

KDAPI
VOID
KdBreakIn(

);

VOID
KdSendPacket(
    _In_ KD_PACKET_TYPE PacketType,
    _In_ PVOID          Buffer,
    _In_ ULONG32        Length
);

KD_STATUS
KdSendPacketMaxRetry(
    _In_ KD_PACKET_TYPE PacketType,
    _In_ PVOID          Buffer,
    _In_ ULONG32        Length,
    _In_ ULONG64        MaxRetries
);

//
// KdPrint is defined inside ntbase.h
//

KDAPI
VOID
KdPrint(
    _In_ PWSTR Format,
    _In_ ...
);
