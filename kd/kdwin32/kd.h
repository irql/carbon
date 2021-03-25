


#pragma once

#undef RtlCopyMemory
#undef RtlMoveMemory

#ifndef __CARBON_H__
typedef struct _PKPROCESS *PKPROCESS;
typedef struct _KTHREAD *PKTHREAD;
typedef struct _MM_VAD *PMM_VAD;
#endif

#pragma warning( disable:4200 )
#pragma pack(push, 1)

#define KD_SIGNATURE '  dK'

typedef enum _KD_STATUS {
    KdStatusSuccess,
    KdStatusError,
    KdStatusTimeout,
    KdStatusFailed,
    KdStatusInvalidChecksum,
    KdStatusMaximum

} KD_STATUS, *PKD_STATUS;

typedef enum _KD_PACKET_TYPE {
    KdPacketInvalid = 0,
    KdPacketAcknowledge,
    KdPacketAcknowledgeFailed,
    KdPacketConnect,

    KdPacketBreakIn,
    KdPacketContinue,
    KdPacketException,
    KdPacketRead,
    KdPacketWrite,
    KdPacketPrint,

    KdPacketAny,
    KdPacketMaximum
} KD_PACKET_TYPE, *PKD_PACKET_TYPE;

typedef struct _KD_PACKET {
    ULONG32 Signature;
    ULONG32 PacketType;
    ULONG32 Checksum;
    ULONG32 Length;
    union {
        struct {
            UCHAR   Data[ 0 ];
        } PacketAny;
        struct {
            union {
                struct {

                    //
                    // Control vars
                    //

                    ULONG64 PsInitialSystemProcess;
                } Initial;
            };
        } PacketConnect;
        struct {
            union {
                struct {
                    ULONG64 Address;
                    ULONG64 Length;
                } Initial;
                struct {
                    UCHAR   Data[ 0 ];
                } Return;
            };
        } PacketRead;
        struct {
            union {
                struct {
                    ULONG64 Address;
                    UCHAR   Data[ 0 ];
                } Initial;
            };
        } PacketWrite;
        struct {
            union {
                struct {
                    WCHAR String[ 0 ];
                } Initial;
            };
        } PacketPrint;
        struct {
            union {
                struct {
                    EXCEPTION_RECORD Record;
                } Initial;
            };
        } PacketException;
    } u[ 0 ];
} KD_PACKET, *PKD_PACKET;

#pragma pack(pop)

EXTERN_C
VOID
KdpInputThread(

);

EXTERN_C
VOID
KdpProcessThread(

);

EXTERN_C
VOID
KdpGetShortName(
    _In_  PWSTR FullName,
    _Out_ PWSTR ShortName
);

EXTERN_C
KD_STATUS
KdpReadDebuggee(
    _In_  ULONG64 Address,
    _In_  ULONG64 Length,
    _Out_ PVOID   Buffer
);

EXTERN_C
KD_STATUS
KdpWriteDebuggee(
    _In_ ULONG64 Address,
    _In_ ULONG64 Length,
    _In_ PVOID   Buffer
);

//
// you're a fat greasy cunt if you call me out for not 
// using templates
//

EXTERN_C
ULONG64
KdpReadULong64(
    _In_ ULONG64 Address
);

EXTERN_C
VOID
KdpWriteULong64(
    _In_ ULONG64 Address,
    _In_ ULONG64 Long
);

EXTERN_C
ULONG32
KdpReadULong32(
    _In_ ULONG64 Address
);

EXTERN_C
VOID
KdpWriteULong32(
    _In_ ULONG64 Address,
    _In_ ULONG32 Long
);

EXTERN_C
USHORT
KdpReadUShort(
    _In_ ULONG64 Address
);

EXTERN_C
VOID
KdpWriteUShort(
    _In_ ULONG64 Address,
    _In_ USHORT  Long
);

EXTERN_C
USHORT
KdpReadUChar(
    _In_ ULONG64 Address
);

EXTERN_C
VOID
KdpWriteUChar(
    _In_ ULONG64 Address,
    _In_ UCHAR   Long
);

EXTERN_C
KD_STATUS
KdSendPacket(
    _In_ KD_PACKET_TYPE PacketType,
    _In_ PVOID          Buffer,
    _In_ ULONG32        Length
);

EXTERN_C
KD_STATUS
KdSendPacketResponse(
    _In_ KD_PACKET_TYPE InitialType,
    _In_ PVOID          InitialBuffer,
    _In_ ULONG32        InitialLength,
    _In_ KD_PACKET_TYPE ReturnType,
    _In_ PVOID          ReturnBuffer,
    _In_ ULONG32        ReturnLength
);

EXTERN_C
ULONG32
KdpComputeChecksum(
    _In_ PVOID   Buffer,
    _In_ ULONG32 Length
);

EXTERN_C
VOID
KdpBreakIn(

);

EXTERN_C
VOID
KdpContinue(

);

extern BOOLEAN KdpBrokenIn;
extern ULONG64 KdpRetryCount;
extern PVOID   KdpProcess;
extern PVOID   KdpSystemProcess;

typedef struct _KD_COMMAND_ARGUMENT {
    ULONG32 ArgumentType;
    union {
        struct {
            PWCHAR String;
        };
        struct {
            LONG64 Integer;
        };
    };
} KD_COMMAND_ARGUMENT, *PKD_COMMAND_ARGUMENT;

typedef enum _KD_ARG_TYPE {
    KdArgumentString = 0,
    KdArgumentInteger,
    KdArgumentComma,
    KdArgumentExclamationPoint,
    KdArgumentEol,
    KdArgumentMaximum

} KD_ARG_TYPE, *PKD_ARG_TYPE;

typedef struct _KD_COMMAND {
    ULONG64             ArgumentCount;
    KD_COMMAND_ARGUMENT Arguments[ 0 ];
} KD_COMMAND, *PKD_COMMAND;

VOID
RtlCopyMemory(
    _In_ PVOID   Destination,
    _In_ PVOID   Source,
    _In_ ULONG64 Length
);
