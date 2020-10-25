


#include "com.h"

LPCWSTR g_KdPipeName = L"\\\\.\\pipe\\KdPipe";
HANDLE g_KdPipe = INVALID_HANDLE_VALUE;

KD_CMD_HANDLER g_CommandTable[ 0xFF ] = {
	KD_DECLARE_NO_HANDLER,
	KD_DECLARE_NO_HANDLER,
	KD_DECLARE_NO_HANDLER,
	KD_DECLARE_HANDLER( KdCmdCrash ),
	KD_DECLARE_HANDLER( KdCmdMessage ),
	KD_DECLARE_HANDLER( KdCmdListThreads ),
	KD_DECLARE_HANDLER( KdCmdListModules ),

};

KD_CMD_STR g_CommandStrings[ 0xFF ] = {
	KD_DECLARE_NO_STRING,
	KD_DECLARE_STRING( L"b" ),
	KD_DECLARE_STRING( L"g" ),
	KD_DECLARE_NO_STRING,
	KD_DECLARE_NO_STRING,
	KD_DECLARE_STRING( L"!tl" ),
	KD_DECLARE_STRING( L"!ml" )

};

VOID
KdPrint(
	__in PWCHAR Buffer,
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

	if ( OutHandle == INVALID_HANDLE_VALUE )
		OutHandle = GetStdHandle( STD_OUTPUT_HANDLE );

	WriteConsoleW( OutHandle, Buffer1, ( DWORD )wcslen( Buffer1 ), &BytesWritten, NULL );

	return;
}

VOID
KdSendCmd(
	__in UCHAR Command
)
{
	DWORD BytesWritten;
	KD_BASE_COMMAND_SEND BaseCmd;

	BaseCmd.KdAckByte = KD_ACK_BYTE;
	BaseCmd.KdCommandByte = Command;

	if ( !WriteFile( g_KdPipe, &BaseCmd, sizeof( KD_BASE_COMMAND_SEND ), &BytesWritten, NULL ) ) {

		KdPrint( L"Command send fail: %d\n", GetLastError( ) );
	}

	return;
}

VOID
KdRecieveThread(

)
{
	CHAR Buffer[ 256 ];

	DWORD BytesRead;
	DWORD Avail;
	while ( 1 ) {

		Sleep( 300 );

		KD_BASE_COMMAND_RECIEVE Recieve;

		if ( !PeekNamedPipe( g_KdPipe, NULL, 0, NULL, &Avail, NULL ) ) {

			continue;
		}

		if ( Avail < sizeof( KD_BASE_COMMAND_RECIEVE ) ) {

			continue;
		}

		PeekNamedPipe( g_KdPipe, &Recieve, sizeof( KD_BASE_COMMAND_RECIEVE ), &BytesRead, NULL, NULL );

		if ( Recieve.KdAckByte != KD_ACK_BYTE ) {

			ReadFile( g_KdPipe, Buffer, 256, &BytesRead, NULL );
			continue;
		}

		while ( Avail != Recieve.KdCmdSize ) {

			PeekNamedPipe( g_KdPipe, NULL, 0, NULL, &Avail, NULL );

			Sleep( 100 );
		}

		PKD_BASE_COMMAND_RECIEVE RecievedCommand = ( PKD_BASE_COMMAND_RECIEVE )HeapAlloc( GetProcessHeap( ), HEAP_ZERO_MEMORY, Recieve.KdCmdSize );

		ReadFile( g_KdPipe, RecievedCommand, Recieve.KdCmdSize, &BytesRead, NULL );

		//KdPrint( L"[ %d ] size: %d\n", RecievedCommand->KdCommandByte, RecievedCommand->KdCmdSize );

		g_CommandTable[ RecievedCommand->KdCommandByte ]( RecievedCommand );

		HeapFree( GetProcessHeap( ), 0, RecievedCommand );
	}

}

VOID
KdEntryPoint(

)
{

	SetConsoleTextAttribute( GetStdHandle( STD_OUTPUT_HANDLE ), 0x0F );

#if 0
	g_KdPipe = CreateNamedPipeW(
		g_KdPipeName,
		PIPE_ACCESS_DUPLEX,
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_ACCEPT_REMOTE_CLIENTS,
		1,
		4096,
		4096,
		0,
		NULL
	);

	SetNamedPipeHandleState( g_KdPipe, NULL, NULL, NULL );
#else
	while ( g_KdPipe == INVALID_HANDLE_VALUE ) {

		g_KdPipe = CreateFileW(
			g_KdPipeName,
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			0,
			NULL
		);

		Sleep( 500 );
	}
#endif

	if ( g_KdPipe == INVALID_HANDLE_VALUE ) {

		ExitProcess( ( unsigned )-1 );
	}

	UCHAR AckByte = 0;
	DWORD BytesRead;
	BOOLEAN ConnectionEstablished = FALSE;

	while ( 1 ) {

		ReadFile( g_KdPipe, &AckByte, 1, &BytesRead, NULL );

		if ( BytesRead == 1 && AckByte == KD_ACK_BYTE ) {

			UCHAR CmdByte = 0;
			ULONG32 Timeout = 100;
			while ( Timeout > 0 ) {
				ReadFile( g_KdPipe, &CmdByte, 1, &BytesRead, NULL );

				if ( BytesRead == 1 && CmdByte == KD_CMD_CONNECT ) {

					ConnectionEstablished = TRUE;
					break;
				}

				Sleep( 10 );
				Timeout--;
			}

			if ( ConnectionEstablished ) {

				break;
			}
		}

		Sleep( 50 );
	}

	//
	//	if code got here, we have a client.
	//

	KdPrint( L"connection established.\n" );

	CreateThread( NULL, 0, ( LPTHREAD_START_ROUTINE )KdRecieveThread, NULL, 0, NULL );

	//Sleep( 250 );

	//KdSendCmd( KD_CMD_BREAK );

	WCHAR Buffer[ 256 ];
	DWORD CharsRead;
	while ( 1 ) {

		ReadConsoleW( GetStdHandle( STD_INPUT_HANDLE ), Buffer, 256, &CharsRead, NULL );

		for ( ULONG32 i = 0; Buffer[ i ]; i++ ) {

			if ( Buffer[ i ] == '\n' || Buffer[ i ] == '\r' ) {

				Buffer[ i ] = 0;
			}
		}

		for ( ULONG32 i = 0; i < 0xFF; i++ ) {

			if ( g_CommandStrings[ i ][ 0 ] != 0 ) {

				if ( lstrcmpiW( Buffer, g_CommandStrings[ i ] ) == 0 ) {

					//KdPrint( L"sent %d\n", i );
					KdSendCmd( ( UCHAR )i );
				}
			}
		}
	}

}
