


#include <intrin.h>
#include <windows.h>
#include "../kd.h"
#include "osl.h"
#include "../pdb.h"

HANDLE  KdpPipe = INVALID_HANDLE_VALUE;
HANDLE  KdpCommunicationLock = INVALID_HANDLE_VALUE;
HANDLE  KdpBreakEvent = INVALID_HANDLE_VALUE;
HANDLE  KdpCacheModuleLock = INVALID_HANDLE_VALUE;

ULONG64 KdpRetryCount = 0;
BOOLEAN KdpBrokenIn = FALSE;
PVOID   KdpProcess = NULL;
PVOID   KdpSystemProcess = NULL;

EXTERN_C
PVOID
OslAllocate(
    _In_ ULONG64 Length
)
{
    return HeapAlloc( GetProcessHeap( ), HEAP_ZERO_MEMORY, Length );
}

EXTERN_C
VOID
OslFree(
    _In_ PVOID Address
)
{
    HeapFree( GetProcessHeap( ), 0, Address );
}

EXTERN_C
VOID
OslWriteConsole(
    __in PCWCHAR Buffer,
    __in ...
)
{
    static HANDLE OutHandle = INVALID_HANDLE_VALUE;

    WCHAR Buffer1[ 1024 ];

    va_list Args;
    va_start( Args, Buffer );

    wvsprintfW( Buffer1, Buffer, Args );

    va_end( Args );

    DWORD BytesWritten;

    if ( OutHandle == INVALID_HANDLE_VALUE ) {

        OutHandle = GetStdHandle( STD_OUTPUT_HANDLE );
    }

    WriteConsoleW( OutHandle, Buffer1, ( DWORD )wcslen( Buffer1 ), &BytesWritten, NULL );

    return;
}

EXTERN_C
ULONG
OslReadConsole(
    _In_ PWSTR Buffer,
    _In_ ULONG Length
)
{
    static HANDLE InHandle = INVALID_HANDLE_VALUE;
    DWORD CharsRead;

    if ( InHandle == INVALID_HANDLE_VALUE ) {

        InHandle = GetStdHandle( STD_INPUT_HANDLE );
    }

    ReadConsoleW( InHandle, Buffer, Length, &CharsRead, NULL );

    return CharsRead;
}

EXTERN_C
VOID
OslSignalBrokeIn(

)
{
    KdpBrokenIn = TRUE;
    SetEvent( KdpBreakEvent );
}

EXTERN_C
VOID
OslSignalContinue(

)
{
    KdpBrokenIn = FALSE;
    ResetEvent( KdpBreakEvent );
}

EXTERN_C
VOID
OslWaitForBreakIn(

)
{
    WaitForSingleObject( KdpBreakEvent, INFINITE );
}

EXTERN_C
VOID
OslAcquireCommunicationLock(

)
{
    WaitForSingleObject( KdpCommunicationLock, INFINITE );
}

EXTERN_C
VOID
OslReleaseCommunicationLock(

)
{
    ReleaseMutex( KdpCommunicationLock );
}

EXTERN_C
VOID
OslAcquireModuleCacheLock(

)
{
    WaitForSingleObject( KdpCacheModuleLock, INFINITE );
}

EXTERN_C
VOID
OslReleaseModuleCacheLock(

)
{
    ReleaseMutex( KdpCacheModuleLock );
}

EXTERN_C
VOID
OslDelayExecution(
    _In_ ULONG Milliseconds
)
{
    Sleep( Milliseconds );
}

EXTERN_C
VOID
OslSendString(
    _In_ PVOID   Buffer,
    _In_ ULONG32 Length
)
{
    DWORD BytesWritten;
    DWORD Avail;

    while ( Length > 0 ) {

        PeekNamedPipe( KdpPipe, NULL, 0, NULL, &Avail, NULL );

        if ( Avail == 0 ) {

            WriteFile( KdpPipe, Buffer, Length >= 0x400 ? 0x400 : Length, &BytesWritten, NULL );

            if ( Length <= 0x400 ) {

                break;
            }

            Length -= 0x400;
        }
        else {

            Sleep( 10 );
        }
    }
}

EXTERN_C
KD_STATUS
OslSendPacket(
    _In_ KD_PACKET_TYPE PacketType,
    _In_ PVOID          Buffer,
    _In_ ULONG32        Length
)
{
    KD_PACKET PacketHeader;

    //OslWriteConsole( L"OslSendPacket %d\n", PacketType );

    PacketHeader.Signature = KD_SIGNATURE;
    PacketHeader.PacketType = PacketType;
    PacketHeader.Length = Length;
    PacketHeader.Checksum = 0;
    PacketHeader.Checksum = KdpComputeChecksum( &PacketHeader, sizeof( KD_PACKET ) );

    if ( Length > 0 ) {

        PacketHeader.Checksum += KdpComputeChecksum( Buffer, Length );
    }

    OslSendString( &PacketHeader, sizeof( KD_PACKET ) );
    if ( Length > 0 ) {

        OslSendString( Buffer, Length );
    }

    return KdStatusSuccess;
}

EXTERN_C
KD_STATUS
OslReceivePacket(
    _In_ KD_PACKET_TYPE PacketType,
    _In_ PVOID          Buffer,
    _In_ ULONG32        Length
)
{
    ULONG32 TimeOut = 0;
    PKD_PACKET Packet;
    DWORD Avail;
    DWORD BytesRead;
    DWORD BytesRead1;
    ULONG Signature;

    Packet = ( PKD_PACKET )Buffer;

    //
    // Change this so it reads a small bit at a time, so that
    // if junk is sent, but a packet is directly after, no data 
    // is discarded as being invalid.
    //

    while ( TimeOut < 10000 ) {

        if ( !PeekNamedPipe( KdpPipe, NULL, 0, NULL, &Avail, NULL ) ) {

            TimeOut++;
            continue;
        }

        if ( Avail < sizeof( ULONG32 ) ) {

            TimeOut++;
            continue;
        }

        PeekNamedPipe( KdpPipe, &Signature, sizeof( ULONG32 ), NULL, NULL, NULL );

        TimeOut = 0;
        if ( Signature != KD_SIGNATURE ) {
            ReadFile( KdpPipe, &Signature, 1, NULL, NULL );
            continue;
        }

        while ( Avail < sizeof( KD_PACKET ) ) {

            PeekNamedPipe( KdpPipe, NULL, 0, NULL, &Avail, NULL );
            Sleep( 100 );
        }

        ReadFile( KdpPipe, Packet, sizeof( KD_PACKET ), &BytesRead, NULL );

        while ( BytesRead < Packet->Length + sizeof( KD_PACKET ) ) { // vuln

            ReadFile( KdpPipe, ( PUCHAR )Packet + BytesRead, Packet->Length - BytesRead + sizeof( KD_PACKET ), &BytesRead1, NULL );
            BytesRead += BytesRead1;
            Sleep( 20 );
        }
        break;
    }

    if ( TimeOut >= 10000 ) {

        return KdStatusTimeout;
    }

    if ( Packet->PacketType == KdPacketAcknowledgeFailed ) {

        return KdStatusError;
    }

    if ( PacketType != KdPacketAny &&
         Packet->PacketType != ( ULONG32 )PacketType ) {

        return KdStatusFailed;
    }

    ULONG32 Checksum = Packet->Checksum;
    Packet->Checksum = 0;
    if ( Checksum != KdpComputeChecksum( Packet, Packet->Length + sizeof( KD_PACKET ) ) ) {

        return KdStatusInvalidChecksum;
    }

    return KdStatusSuccess;
}

EXTERN_C
BOOLEAN
OslConsoleEvent(
    _In_ DWORD Signal
)
{
    switch ( Signal ) {
    case CTRL_C_EVENT:
        if ( !KdpBrokenIn ) {

            //OslWriteConsole( L"break attempt..\n" );
            //KdSendPacket( KdPacketBreakIn, NULL, 0 );
            //KdpBrokenIn = TRUE;
            //KdpBreakIn( );
        }
        return TRUE;
    default:
        return FALSE;
    }
}

EXTERN_C
VOID
OslProcessStartup(

)
{
    SetConsoleTextAttribute( GetStdHandle( STD_OUTPUT_HANDLE ), 0x0F );
    SetConsoleCtrlHandler( ( PHANDLER_ROUTINE )OslConsoleEvent, TRUE );

    CONSOLE_FONT_INFOEX Font;

    Font.cbSize = sizeof( CONSOLE_FONT_INFOEX );
    Font.nFont = 0;
    Font.dwFontSize.X = 0;
    Font.dwFontSize.Y = 16;
    Font.FontFamily = FF_DONTCARE;
    Font.FontWeight = FW_NORMAL;
    lstrcpyW( Font.FaceName, L"Envy Code R" );
    SetCurrentConsoleFontEx( GetStdHandle( STD_OUTPUT_HANDLE ), FALSE, &Font );

#if 1
    //
    // Server. (read/send is optimized for vmwares 0x400 buffer size)
    //

    while ( KdpPipe == INVALID_HANDLE_VALUE ) {

        KdpPipe = CreateFileW( L"\\\\.\\pipe\\KdPipe",
                               GENERIC_READ | GENERIC_WRITE,
                               0,
                               NULL,
                               OPEN_EXISTING,
                               0,
                               NULL );

        Sleep( 500 );
    }
#else
    //
    // Client.
    //

    KdpPipe = CreateNamedPipeW( L"\\\\.\\pipe\\KdPipe",
                                PIPE_ACCESS_DUPLEX,
                                PIPE_TYPE_BYTE | PIPE_READMODE_BYTE |
                                PIPE_WAIT | PIPE_ACCEPT_REMOTE_CLIENTS,
                                1,
                                4096,
                                4096,
                                0,
                                NULL );
#endif

    KdpBreakEvent = CreateEventW( NULL, TRUE, FALSE, NULL );
    KdpCacheModuleLock = CreateMutexW( NULL, FALSE, NULL );
    KdpCommunicationLock = CreateMutexW( NULL, FALSE, NULL );

    OslWriteConsole( L"pipe opened.\n" );

    KD_STATUS  Status;
    PKD_PACKET Packet = ( PKD_PACKET )_alloca( sizeof( KD_PACKET ) + sizeof( ULONG64 ) );

    //
    // TODO: Improve connect/re-connect shit like that
    // re-connect worked for like 1 day and then it broke
    //

    while ( 1 ) {

        Status = OslReceivePacket( KdPacketConnect,
                                   Packet,
                                   sizeof( KD_PACKET ) + sizeof( ULONG64 ) );

        if ( Status == KdStatusSuccess ) {

            KdpSystemProcess = ( PVOID )Packet->u[ 0 ].PacketConnect.Initial.PsInitialSystemProcess;
            KdpProcess = KdpSystemProcess;

            OslSendPacket( KdPacketAcknowledge, NULL, 0 );
            break;
        }

        Sleep( 50 );
    }

    OslWriteConsole( L"connected.\n" );

#if 0
    //if ( KdSendPacketMaxRetry( KdPacketConnect, NULL, 0, 0 ) != KdStatusSuccess ) {
    OslSendPacket( KdPacketConnect, NULL, 0 );

    if ( OslReceivePacket( KdPacketAcknowledge, &Packet, sizeof( KD_PACKET ) != KdStatusSuccess ) ) {

        while ( 1 ) {

            Status = OslReceivePacket( KdPacketConnect,
                                       &Packet,
                                       sizeof( KD_PACKET ) );

            if ( Status == KdStatusSuccess ) {

                OslSendPacket( KdPacketAcknowledge, NULL, 0 );
                break;
            }

            Sleep( 50 );
        }

        OslWriteConsole( L"connected.\n" );
    }
    else {

        OslWriteConsole( L"quick connect.\n" );
    }
#endif

    DbgInitialize( );

    //Sleep( 500 );

    CreateThread( NULL,
                  0x40000,
                  ( LPTHREAD_START_ROUTINE )KdpInputThread,
                  NULL,
                  0,
                  NULL );

    //Sleep( 200 );

    CreateThread( NULL,
                  0x40000,
                  ( LPTHREAD_START_ROUTINE )KdpProcessThread,
                  NULL,
                  0,
                  NULL );

    Sleep( -1 );
    //ExitThread( 0 );
}
