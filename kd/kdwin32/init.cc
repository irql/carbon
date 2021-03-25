


#include <windows.h>
#include <intrin.h>
#include "kd.h"
#include "pdb.h"
#include "osl/osl.h"

EXTERN_C
ULONG32
KdpComputeChecksum(
    _In_ PVOID   Buffer,
    _In_ ULONG32 Length
)
{
    ULONG32 Result;
    PUCHAR  CharBuffer;

    CharBuffer = ( PUCHAR )Buffer;
    Result = 0;
    while ( Length-- ) {

        Result += *CharBuffer++;
    }

    return Result;
}

EXTERN_C
KD_STATUS
KdSendPacket(
    _In_ KD_PACKET_TYPE PacketType,
    _In_ PVOID          Buffer,
    _In_ ULONG32        Length
)
{
    ULONG64 TimeOut = 0;
    KD_PACKET Packet;
    KD_STATUS Status;

    KdpRetryCount = 0;

PacketRetry:;

    OslSendPacket( PacketType, Buffer, Length );
    do {
        Status = OslReceivePacket( KdPacketAcknowledge,
                                   &Packet,
                                   sizeof( KD_PACKET ) );
        if ( Status == KdStatusError ) {

            return KdStatusError;
        }

        OslDelayExecution( 50 );
        TimeOut++;
    } while ( TimeOut < 15 && Status != KdStatusSuccess );

    if ( TimeOut < 15 ) {

        return KdStatusSuccess;
    }

    TimeOut = 0;
    KdpRetryCount++;
    OslDelayExecution( 100 );
    goto PacketRetry;

}

EXTERN_C
KD_STATUS
KdSendPacketMaxRetry(
    _In_ KD_PACKET_TYPE PacketType,
    _In_ PVOID          Buffer,
    _In_ ULONG32        Length,
    _In_ ULONG64        MaxRetries
)
{
    ULONG64 TimeOut = 0;
    KD_PACKET Packet;

    KdpRetryCount = 0;

PacketRetry:;

    OslSendPacket( PacketType, Buffer, Length );
    while ( TimeOut < 15 &&
            OslReceivePacket( KdPacketAcknowledge,
                              &Packet,
                              sizeof( KD_PACKET ) ) != KdStatusSuccess ) {
        OslDelayExecution( 50 );
        TimeOut++;
    }

    if ( TimeOut < 15 ) {

        return KdStatusSuccess;
    }

    TimeOut = 0;
    KdpRetryCount++;

    if ( KdpRetryCount > MaxRetries ) {

        return KdStatusTimeout;
    }

    OslDelayExecution( 100 );
    goto PacketRetry;

}

EXTERN_C
KD_STATUS
KdSendPacketResponse(
    _In_ KD_PACKET_TYPE InitialType,
    _In_ PVOID          InitialBuffer,
    _In_ ULONG32        InitialLength,
    _In_ KD_PACKET_TYPE ReturnType,
    _In_ PVOID          ReturnBuffer,
    _In_ ULONG32        ReturnLength
)
{
    ULONG64 TimeOut;

    KdpRetryCount = 0;
    TimeOut = 0;

PacketRetry:;

    if ( KdSendPacket( InitialType,
                       InitialBuffer,
                       InitialLength ) == KdStatusError ) {

        return KdStatusError;
    }

    while ( TimeOut < 15 &&
            OslReceivePacket( ReturnType,
                              ReturnBuffer,
                              ReturnLength ) != KdStatusSuccess ) {

        OslDelayExecution( 50 );
        TimeOut++;
    }

    if ( TimeOut < 15 ) {

        OslSendPacket( KdPacketAcknowledge, NULL, 0 );
        return KdStatusSuccess;
    }

    TimeOut = 0;
    KdpRetryCount++;
    OslDelayExecution( 100 );
    goto PacketRetry;
}

EXTERN_C
VOID
KdpBreakIn(

)
{
    OslAcquireCommunicationLock( );
#if 1
    KdSendPacket( KdPacketBreakIn, NULL, 0 );
#else
    KD_PACKET Packet;
    OslSendPacket( KdPacketBreakIn, NULL, 0 );
    while ( OslReceivePacket( KdPacketAcknowledge,
                              &Packet,
                              sizeof( KD_PACKET ) ) != KdStatusSuccess );
#endif
    OslReleaseCommunicationLock( );
    OslWriteConsole( L"debug break...\n" );
    OslSignalBrokeIn( );
}

EXTERN_C
VOID
KdpContinue(

)
{
    OslAcquireCommunicationLock( );
#if 1
    KdSendPacket( KdPacketContinue, NULL, 0 );
#else
    KD_PACKET Packet;
    OslSendPacket( KdPacketContinue, NULL, 0 );
    while ( OslReceivePacket( KdPacketAcknowledge,
                              &Packet,
                              sizeof( KD_PACKET ) ) != KdStatusSuccess );
#endif
    OslReleaseCommunicationLock( );
    OslWriteConsole( L"continued...\n" );
    OslSignalContinue( );
}
