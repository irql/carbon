


#define __KD_COM__
#include <carbsup.h>
#include "kdp.h"
#include "kd.h"
#include "../../carbkrnl/hal/halp.h"
#include "../../carbkrnl/ke/ki.h"
#include "../../carbkrnl/mm/mi.h"

NTSTATUS
DriverLoad(
    _In_ PDRIVER_OBJECT DriverObject
);

KD_CONTEXT        KdpDebuggerContext;
USHORT            KdpDefaultPort = 0x3F8;
BOOLEAN           KdpDebuggerConnected;
KSPIN_LOCK        KdpCommunicationLock = 0;
PEXCEPTION_RECORD KdpCurrentException = NULL;
BOOLEAN           KdpResendException = FALSE;

ULONG32
KdpComputeChecksum(
    _In_ PVOID   Buffer,
    _In_ ULONG32 Length
)
{
    ULONG32 Result;
    PUCHAR CharBuffer;

    CharBuffer = Buffer;
    Result = 0;
    while ( Length-- ) {

        Result += *CharBuffer++;
    }

    return Result;
}

BOOLEAN
KdpReceiveReady(

)
{

    return ( __inbyte( KdpDefaultPort + COM_LINE_STATUS_REG ) & COM_LS_DR ) == COM_LS_DR;
}

UCHAR
KdpReceiveChar(

)
{
    while ( !KdpReceiveReady( ) )
        ;

    //yeap, osdev debugging
    //RtlDebugPrint( L"Char: %d %d %d %d\n", p, PsGetThreadId( PsGetCurrentThread( ) ), KeGetCurrentIrql( ), KeQueryCurrentProcessor( )->ProcessorNumber );
    return __inbyte( KdpDefaultPort + COM_DATA_REG );
}

VOID
KdpReadString(
    _In_ PVOID   Buffer,
    _In_ ULONG32 Length
)
{
    while ( Length-- ) {

        *( ( PUCHAR )Buffer ) = KdpReceiveChar( );
        ( ( PUCHAR )Buffer )++;
    }
}

BOOLEAN
KdpSendReady(

)
{

    return ( __inbyte( KdpDefaultPort + COM_LINE_STATUS_REG ) & COM_LS_THRE ) == COM_LS_THRE;
}

VOID
KdpSendChar(
    _In_ UCHAR Char
)
{
    while ( !KdpSendReady( ) )
        ;

    __outbyte( KdpDefaultPort + COM_DATA_REG, Char );
}

VOID
KdpSendString(
    _In_ PVOID   Buffer,
    _In_ ULONG32 Length
)
{
    while ( Length-- ) {

        KdpSendChar( *( PUCHAR )Buffer );
        ( ( PUCHAR )Buffer )++;
    }
}

KD_STATUS
KdpSendPacket(
    _In_ KD_PACKET_TYPE PacketType,
    _In_ PVOID          Buffer,
    _In_ ULONG32        Length
)
{
    KD_PACKET PacketHeader;

    PacketHeader.Signature = KD_SIGNATURE;
    PacketHeader.PacketType = PacketType;
    PacketHeader.Length = Length;
    PacketHeader.Checksum = 0;
    PacketHeader.Checksum = KdpComputeChecksum( &PacketHeader, sizeof( KD_PACKET ) );

    if ( Length > 0 ) {

        PacketHeader.Checksum += KdpComputeChecksum( Buffer, Length );
    }

    KdpSendString( &PacketHeader, sizeof( KD_PACKET ) );
    if ( Length > 0 ) {

        KdpSendString( Buffer, Length );
    }

    return KdStatusSuccess;
}

KD_STATUS
KdpReceivePacket(
    _In_ KD_PACKET_TYPE PacketType,
    _In_ PVOID          Buffer,
    _In_ ULONG32        Length
)
{
    Length;
    KD_STATUS Status;
    ULONG32 TimeOut;
    ULONG32 LengthRecv;
    PKD_PACKET Packet;

    Status = KdStatusTimeout;
    TimeOut = 0;
    LengthRecv = 0;
    Packet = Buffer;

    //
    // Change this so it reads a small bit at a time, so that
    // if junk is sent, but a packet is directly after, no data 
    // is discarded as being invalid.
    //

    RtlZeroMemory( Packet, sizeof( KD_PACKET ) );

    while ( TimeOut < 100000 &&
            LengthRecv < sizeof( KD_PACKET ) ) {

        if ( !KdpReceiveReady( ) ) {

            TimeOut++;
            continue;
        }

        *( PUCHAR )( ( PUCHAR )Buffer + LengthRecv ) = KdpReceiveChar( );
        LengthRecv++;
        TimeOut = 0;

        if ( LengthRecv == 1 &&
            ( Packet->Signature & 0xFF ) != ( KD_SIGNATURE & 0xFF ) ) {
            LengthRecv = 0;
            Packet->Signature = 0;
            //RtlDebugPrint( L"Char: %d %d %d %d\n", Packet->Signature, PsGetThreadId( PsGetCurrentThread( ) ), KeGetCurrentIrql( ), KeQueryCurrentProcessor( )->ProcessorNumber );
            Status = KdStatusFailed;
        }

        if ( LengthRecv == 2 &&
            ( Packet->Signature & 0xFF00 ) != ( KD_SIGNATURE & 0xFF00 ) ) {
            LengthRecv = 0;
            Packet->Signature = 0;
            //RtlDebugPrint( L"Char: %d %d %d %d\n", Packet->Signature, PsGetThreadId( PsGetCurrentThread( ) ), KeGetCurrentIrql( ), KeQueryCurrentProcessor( )->ProcessorNumber );
            Status = KdStatusFailed;
        }

        if ( LengthRecv == 3 &&
            ( Packet->Signature & 0xFF0000 ) != ( KD_SIGNATURE & 0xFF0000 ) ) {
            LengthRecv = 0;
            Packet->Signature = 0;
            //RtlDebugPrint( L"Char: %d %d %d %d\n", Packet->Signature, PsGetThreadId( PsGetCurrentThread( ) ), KeGetCurrentIrql( ), KeQueryCurrentProcessor( )->ProcessorNumber );
            Status = KdStatusFailed;
        }

        if ( LengthRecv == 4 &&
            ( Packet->Signature & 0xFF000000 ) != ( KD_SIGNATURE & 0xFF000000 ) ) {
            LengthRecv = 0;
            Packet->Signature = 0;
            Status = KdStatusFailed;
        }
    }

    if ( TimeOut >= 100000 ) {

        return Status;
    }

    if ( PacketType != KdPacketAny && Packet->PacketType != ( ULONG32 )PacketType ) {

        return KdStatusFailed;
    }

    //
    // Read the rest of the packet, the timeout is much
    // larger because we already have a valid header.
    //

    TimeOut = 0;
    while ( TimeOut < 100000 &&
            LengthRecv < Packet->Length + sizeof( KD_PACKET ) &&
            LengthRecv < Length ) {

        if ( !KdpReceiveReady( ) ) {

            //TimeOut++;
            continue;
        }

        TimeOut = 0;
        *( PUCHAR )( ( PUCHAR )Buffer + LengthRecv ) = KdpReceiveChar( );
        LengthRecv++;
    }

    if ( TimeOut >= 100000 ) {

        return KdStatusTimeout;
    }

    ULONG32 Checksum = Packet->Checksum;
    Packet->Checksum = 0;
    if ( Checksum != KdpComputeChecksum( Packet, Packet->Length + sizeof( KD_PACKET ) ) ) {

        return KdStatusInvalidChecksum;
    }

    return KdStatusSuccess;
}

//
// Implement KdReceivePacket too
//

VOID
KdSendPacket(
    _In_ KD_PACKET_TYPE PacketType,
    _In_ PVOID          Buffer,
    _In_ ULONG32        Length
)
{
    ULONG64 TimeOut = 0;
    KD_PACKET Packet;

    KdpDebuggerContext.RetryCount = 0;

PacketRetry:;

    KdpSendPacket( PacketType, Buffer, Length );
    while ( TimeOut < 1000 &&
            KdpReceivePacket( KdPacketAcknowledge,
                              &Packet,
                              sizeof( KD_PACKET ) ) != KdStatusSuccess ) {

        TimeOut++;
    }

    if ( TimeOut < 1000 ) {

        return;
    }

    TimeOut = 0;
    KdpDebuggerContext.RetryCount++;
    //HalDelayExecutionPit( 10 );
    goto PacketRetry;

}

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

    KdpDebuggerContext.RetryCount = 0;

PacketRetry:;

    KdpSendPacket( PacketType, Buffer, Length );
    while ( TimeOut < 1000 &&
            KdpReceivePacket( KdPacketAcknowledge,
                              &Packet,
                              sizeof( KD_PACKET ) ) != KdStatusSuccess ) {

        TimeOut++;
    }

    if ( TimeOut < 1000 ) {

        return KdStatusSuccess;
    }

    TimeOut = 0;
    KdpDebuggerContext.RetryCount++;

    if ( KdpDebuggerContext.RetryCount > MaxRetries ) {

        return KdStatusFailed;
    }

    goto PacketRetry;

}

VOID
KdDispatchException(
    _In_ PEXCEPTION_RECORD Exception
)
{
    KIRQL PreviousIrql;

    if ( KdpCurrentException != NULL ) {

        //
        // Kd fault.
        //

        RtlDebugPrint( L":WhenTheDebuggerCrashes:\n" );

        return;
    }

    KeRaiseIrql( DISPATCH_LEVEL, &PreviousIrql );

    KdpBreakIn( );

    //
    // This lock must be free already, but we can just set
    // it to 0 incase, (it is an interrupt-safe lock, the ipi
    // will only be delivered once it's released)
    // 

    KdpCommunicationLock = 0;
    //KeAcquireSpinLockAtDpcLevel( &KdpCommunicationLock );
    //IoAcquireInterruptSafeLock( &KdpCommunicationLock );
    //IoReleaseInterruptSafeLock( &KdpCommunicationLock );

    //
    // Tell it we've broke in, and send the exception record
    //

#if 0
    KdpResendException =
        KdSendPacketMaxRetry( KdPacketBreakIn, NULL, 0, 4 ) != KdStatusSuccess ||
        KdSendPacketMaxRetry( KdPacketException, Exception, sizeof( EXCEPTION_RECORD ), 4 ) != KdStatusSuccess;
    KdpCurrentException = Exception;
#endif

    KdSendPacket( KdPacketBreakIn, NULL, 0 );

    KdpResendException = KdSendPacketMaxRetry( KdPacketException, Exception, sizeof( EXCEPTION_RECORD ), 4 ) != KdStatusSuccess;
    KdpCurrentException = Exception;


    //
    // Release the communication lock, KdpInputThread will attempt
    // to acquire it.
    //

    //KeReleaseSpinLockAtDpcLevel( &KdpCommunicationLock );

    KdpInputThread( &KdpDebuggerContext.BreakRequested );
    KdpCurrentException = NULL;

    KeLowerIrql( PreviousIrql );
}

VOID
KdBreakIn(

)
{
    //KIRQL PreviousIrql;

#if 0
    KeAcquireSpinLock( &KdpCommunicationLock, &PreviousIrql );
#else
    IoAcquireInterruptSafeLock( &KdpCommunicationLock );
#endif

    KdSendPacket( KdPacketBreakIn, NULL, 0 );

    KdpBreakIn( );

    // brutal?
    IoReleaseInterruptSafeLock( &KdpCommunicationLock );

    KdpInputThread( &KdpDebuggerContext.BreakRequested );

#if 0
    KeReleaseSpinLock( &KdpCommunicationLock, PreviousIrql );
#endif
}

VOID
KdpBreakProcessor(
    _In_ PKD_IPI_CONTEXT Context
)
{

    //
    // optimize cpu usage with KiSleepIdleProcessor or something
    //

    if ( KeQueryCurrentProcessor( )->ProcessorNumber == Context->ProcessorSkip ) {

        return;
    }

    _InterlockedIncrement64( ( LONG64* )&Context->ProcessorControl );

    while ( KdpDebuggerContext.BreakRequested )
        ;
}

VOID
KdpBreakIn(

)
{
    KD_IPI_CONTEXT Context;

    Context.ProcessorControl = 0;
    Context.ProcessorSkip = KeQueryCurrentProcessor( )->ProcessorNumber;
    KdpDebuggerContext.BreakRequested = TRUE;

    KeGenericCallIpi( KdpBreakProcessor, &Context );

    while ( Context.ProcessorControl != ( KeQueryProcessorCount( ) - 1 ) )
        ;
}

VOID
KdpContinue(

)
{
    KdpDebuggerContext.BreakRequested = FALSE;
}

VOID
KdContinue(

)
{
    //KIRQL PreviousIrql;

#if 0
    KeAcquireSpinLock( &KdpCommunicationLock, &PreviousIrql );
#else
    IoAcquireInterruptSafeLock( &KdpCommunicationLock );
#endif

    KdSendPacketMaxRetry( KdPacketContinue, NULL, 0, 4 );

#if 0
    KeReleaseSpinLock( &KdpCommunicationLock, PreviousIrql );
#else
    IoReleaseInterruptSafeLock( &KdpCommunicationLock );
#endif

    KdpContinue( );
}

VOID
KdpInputThread(
    _In_ PBOOLEAN Condition
)
{
    BOOLEAN    Acknowledge;
    BOOLEAN    LocalBreak;
    CHAR       Buffer[ 8192 ];
    PKD_PACKET Packet = ( PKD_PACKET )&Buffer;//_alloca( 8192 );
    KIRQL      InitialIrql;
    KIRQL      DiscardIrql;
    ULONG64    ReadAddress;
    ULONG64    ReadLength;
    ULONG64    TimeSinceConnect;
    ULONG64    PreviousAddressSpace;
    //BOOLEAN    ServiceState;

    //
    // The LocalBreak variable makes sure that 
    // if an exception occurs on a different processor
    // the debuggee, can handle the exception and take control of this thread
    // ultimately, preventing this thread from raising its irql too high to 
    // receive a dpc, which is what KdpBreakProcessor uses.
    //
    // Both the client and server need this fix: Create a thread which dispatches
    // kernel debugger requests from a queue and have another thread read and queue 
    // the requests. (maybe?)
    //

    LocalBreak = FALSE;
    TimeSinceConnect = 0;
    InitialIrql = KeGetCurrentIrql( );

    while ( *Condition ) {

        //
        // Irql checks are in place because when the debugger breaks in, it
        // takes control of each processor and calls this api at dispatch_level.
        //

        if ( !KdpReceiveReady( ) ) {

            //HalDelayExecutionPit( 100 );
            //KiLeaveQuantumEarly( );
            continue;
        }

#if 0
        if ( !KeQueryCurrentProcessor( )->InService ) {
            if ( LocalBreak ) {

                KeAcquireSpinLockAtDpcLevel( &KdpCommunicationLock );
            }
            else {

                KeAcquireSpinLock( &KdpCommunicationLock, &DiscardIrql );
            }
        }
#else
        IoAcquireInterruptSafeLock( &KdpCommunicationLock );
#endif

        if ( KdpReceivePacket( KdPacketAny, Packet, 8192 ) == KdStatusSuccess ) {

            //
            // TimeSinceConnect is useless because of Connect packet changes.
            // TODO: Cleanup.
            //

            //RtlDebugPrint( L"Packet: %d\n", Packet->PacketType );

            Acknowledge = TRUE;
            switch ( Packet->PacketType ) {
            case KdPacketBreakIn:
                //KdpSendPacket( KdPacketAcknowledge, NULL, 0 );
                //Acknowledge = FALSE;
                /*
                KdSendPacket( KdPacketBreakIn,
                              NULL,
                              0 );*/

                LocalBreak = TRUE;
                NT_ASSERT( !KdpDebuggerContext.BreakRequested );
                KeRaiseIrql( DISPATCH_LEVEL, &DiscardIrql );
                KdpBreakIn( );
                //__debugbreak( );
                break;
            case KdPacketContinue:

                LocalBreak = FALSE;
                KdpContinue( );
                KeLowerIrql( InitialIrql );
                break;
            case KdPacketRead:

                ReadAddress = Packet->u[ 0 ].PacketRead.Initial.Address;
                ReadLength = Packet->u[ 0 ].PacketRead.Initial.Length;
                PreviousAddressSpace = MiGetAddressSpace( );

                __try {

                    MiSetAddressSpace( ( ( PKPROCESS )Packet->u[ 0 ].PacketRead.Initial.Process )->DirectoryTableBase );
                    RtlCopyMemory( &Packet->u[ 0 ].PacketRead.Return.Data, ( PVOID )ReadAddress, ReadLength );
                }
                __except ( EXCEPTION_EXECUTE_HANDLER ) {
#if 0
                    RtlDebugPrint( L"[kdcom] read failed: addr=%ull length=%ull process=%ull\n",
                                   ReadAddress,
                                   ReadLength,
                                   Packet->u[ 0 ].PacketRead.Initial.Process );
#endif
                    KdpSendPacket( KdPacketAcknowledgeFailed, NULL, 0 );
                    Acknowledge = FALSE;
                }

                MiSetAddressSpace( PreviousAddressSpace );

                if ( Acknowledge ) {

                    KdpSendPacket( KdPacketAcknowledge, NULL, 0 );
                    Acknowledge = FALSE;

                    KdSendPacketMaxRetry( KdPacketRead,
                                          &Packet->u[ 0 ].PacketRead.Return.Data,
                                          ( ULONG32 )ReadLength,
                                          4 );
                }

                break;
            case KdPacketWrite:

                //
                // currently, this just uses the Ack to tell if it was successful.
                //

                PreviousAddressSpace = MiGetAddressSpace( );

                __try {

                    MiSetAddressSpace( ( ( PKPROCESS )Packet->u[ 0 ].PacketWrite.Initial.Process )->DirectoryTableBase );
                    RtlCopyMemory( ( PVOID )Packet->u[ 0 ].PacketWrite.Initial.Address, Packet->u[ 0 ].PacketWrite.Initial.Data, Packet->Length - 8 );
                }
                __except ( EXCEPTION_EXECUTE_HANDLER ) {

                    RtlDebugPrint( L"[kdcom] write failed: address=%ull length=%ull process=%ull\n",
                                   Packet->u[ 0 ].PacketWrite.Initial.Address,
                                   Packet->Length,
                                   Packet->u[ 0 ].PacketWrite.Initial.Process );

                    KdpSendPacket( KdPacketAcknowledgeFailed, NULL, 0 );
                    Acknowledge = FALSE;
                }

                MiSetAddressSpace( PreviousAddressSpace );

                break;
            case KdPacketConnect:
                KdpSendPacket( KdPacketAcknowledge, NULL, 0 );
                Acknowledge = FALSE;

                HalDelayExecutionPit( 100 );
                TimeSinceConnect = 1;

                break;
            default:

                //
                // Don't send an ack, we don't recognise it
                // it should try to retry and eventually give up.
                //
                Acknowledge = FALSE;
                break;
            }

            if ( Acknowledge ) {

                KdpSendPacket( KdPacketAcknowledge, NULL, 0 );
            }

        }

        if ( TimeSinceConnect > 0 ) {

            TimeSinceConnect++;
        }

        if ( ( KdpCurrentException != NULL ) && ( KdpResendException || TimeSinceConnect > 4 ) ) {
            KdpResendException =
                KdSendPacketMaxRetry( KdPacketBreakIn, NULL, 0, 4 ) != KdStatusSuccess ||
                KdSendPacketMaxRetry( KdPacketException, KdpCurrentException, sizeof( EXCEPTION_RECORD ), 4 ) != KdStatusSuccess;
            TimeSinceConnect = 0;
        }
#if 0
        if ( !KeQueryCurrentProcessor( )->InService ) {
            if ( LocalBreak ) {

                KeReleaseSpinLockAtDpcLevel( &KdpCommunicationLock );
            }
            else {

                KeReleaseSpinLock( &KdpCommunicationLock, InitialIrql );
            }
        }
#else
        IoReleaseInterruptSafeLock( &KdpCommunicationLock );
#endif


        if ( !LocalBreak ) {

            // insert delay api
            //HalDelayExecutionPit( 100 );
        }
    }
}

VOID
KdPrint(
    _In_ PWSTR Format,
    _In_ ...
)
{
    //KIRQL     PreviousIrql;
    WCHAR     Buffer[ 256 ];
    VA_LIST   ArgList;

    if ( !KdDebuggerEnabled || KdpCurrentException != NULL ) {

        return;
    }

    __crt_va_start( ArgList, Format );
    RtlFormatBufferFromArgumentList( Buffer, Format, ArgList );
    __crt_va_end( ArgList );
#if 0
    if ( !KeQueryCurrentProcessor( )->InService ) {
        KeAcquireSpinLock( &KdpCommunicationLock, &PreviousIrql );
    }
#else
    IoAcquireInterruptSafeLock( &KdpCommunicationLock );
#endif
    KdSendPacketMaxRetry( KdPacketPrint, Buffer, ( lstrlenW( Buffer ) + 1 ) * sizeof( WCHAR ), 4 );
#if 0
    if ( !KeQueryCurrentProcessor( )->InService ) {
        KeReleaseSpinLock( &KdpCommunicationLock, PreviousIrql );
    }
#else
    IoReleaseInterruptSafeLock( &KdpCommunicationLock );
#endif
}

NTSTATUS
DriverLoad(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    DriverObject;

    STATIC BOOLEAN KdpInputThreadEnable = TRUE;
    //HANDLE ThreadHandle;

    __outbyte( KdpDefaultPort + COM_INTERRUPT_ENABLE_REG, 0 );

    __outbyte( KdpDefaultPort + COM_LINE_CONTROL_REG, COM_LC_DLAB );

    __outbyte( KdpDefaultPort + COM_LSB_BAUD_RATE, 1 );
    __outbyte( KdpDefaultPort + COM_MSB_BAUD_RATE, 0 );

    __outbyte( KdpDefaultPort + COM_LINE_CONTROL_REG, 3 );

    __outbyte( KdpDefaultPort + COM_INT_IDENT_FIFO_CR, 0xC7 );
    __outbyte( KdpDefaultPort + COM_MODEM_CONTROL_REG, 0xB );

    KD_PACKET Packet;

    //if ( KdpReceivePacket( KdPacketConnect, &Packet, sizeof( KD_PACKET ) ) == KdStatusSuccess ) {

    //    KdpSendPacket( KdPacketAcknowledge, &Packet, sizeof( KD_PACKET ) );
    //    KdpDebuggerConnected = 1;
    //}
    //else {

    KdpSendPacket( KdPacketConnect, &PsInitialSystemProcess, sizeof( ULONG64 ) );
    KdpDebuggerConnected = KdpReceivePacket( KdPacketAcknowledge, &Packet, sizeof( KD_PACKET ) ) == KdStatusSuccess;
    // }
    KdDebuggerEnabled = KdpDebuggerConnected;

    //RtlDebugPrint( L"KdpDebuggerConnected: %d\n", KdpDebuggerConnected );

    if ( KdpDebuggerConnected ) {
#if 0
        OBJECT_ATTRIBUTES Thread = { { 0 }, { 0 }, OBJ_KERNEL_HANDLE };
        ZwCreateThread(
            &ThreadHandle,
            ZwCurrentProcess( ),
            THREAD_ALL_ACCESS,
            ( PKSTART_ROUTINE )KdpInputThread,
            &KdpInputThreadEnable,
            THREAD_SYSTEM,
            &Thread,
            0x4000,
            NULL );
#endif
        //HalDelayExecutionPit( 10 );

#if __KD_INSTANT_BREAK__

        __debugbreak( );
#endif

        //KdPrint( L"kdcom says hi.\n" );
    }

    return STATUS_SUCCESS;
}
