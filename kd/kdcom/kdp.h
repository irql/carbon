


#pragma once

#define __KD_INSTANT_BREAK__ 0

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
                    ULONG64 Process;
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
                    ULONG64 Process;
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

#define COM_DATA_REG 0
#define COM_INTERRUPT_ENABLE_REG 1

#define COM_LSB_BAUD_RATE 0
#define COM_MSB_BAUD_RATE 1

#define COM_INT_IDENT_FIFO_CR 2
#define COM_LINE_CONTROL_REG 3 //MSB IS THE DLAB
#define COM_MODEM_CONTROL_REG 4
#define COM_LINE_STATUS_REG 5
#define COM_MODEM_STATUS_REG 6
#define COM_SCRATCH_REG 7

#define COM_LC_DLAB     ( 1 << 7 )

#define COM_LS_DR       ( 1 << 0 )
#define COM_LS_OE       ( 1 << 0 )
#define COM_LS_PE       ( 1 << 2 )
#define COM_LS_FE       ( 1 << 3 )
#define COM_LS_BI       ( 1 << 4 )
#define COM_LS_THRE     ( 1 << 5 )
#define COM_LS_TEMT     ( 1 << 6 )
#define COM_LS_ER_INP   ( 1 << 7 )

#define COM_LC_DB_5 0
#define COM_LC_DB_6 1
#define COM_LC_DB_7 2
#define COM_LC_DB_8 3

#define COM_LC_SB_1 0
#define COM_LC_SB_2 (1 << 2)

typedef struct _KD_CONTEXT {
    ULONG64 RetryCount;
    BOOLEAN BreakRequested;

} KD_CONTEXT, *PKD_CONTEXT;

typedef struct _KD_IPI_CONTEXT {
    ULONG64 ProcessorControl;
    ULONG64 ProcessorSkip;

} KD_IPI_CONTEXT, *PKD_IPI_CONTEXT;

EXTERN KD_CONTEXT KdpDebuggerContext;
EXTERN USHORT     KdpDefaultPort;
EXTERN BOOLEAN    KdpDebuggerConnected;

ULONG32
KdpComputeChecksum(
    _In_ PVOID   Buffer,
    _In_ ULONG32 Length
);

BOOLEAN
KdpReceiveReady(

);

UCHAR
KdpReceiveChar(

);

VOID
KdpReadString(
    _In_ PVOID   Buffer,
    _In_ ULONG32 Length
);

BOOLEAN
KdpSendReady(

);

VOID
KdpSendChar(
    _In_ UCHAR Char
);

VOID
KdpSendString(
    _In_ PVOID   Buffer,
    _In_ ULONG32 Length
);

KD_STATUS
KdpSendPacket(
    _In_ KD_PACKET_TYPE PacketType,
    _In_ PVOID          Buffer,
    _In_ ULONG32        Length
);

KD_STATUS
KdpReceivePacket(
    _In_ KD_PACKET_TYPE PacketType,
    _In_ PVOID          Buffer,
    _In_ ULONG32        Length
);

VOID
KdpInputThread(
    _In_ PBOOLEAN Condition
);

VOID
KdpBreakIn(

);

VOID
KdpContinue(

);
