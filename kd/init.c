


#include "com.h"

#define toupper(v)			((v) >= 'a' && (v) <= 'z' ? (v) - ' ' : (v))

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
	KD_DECLARE_HANDLER( KdCmdThreadContext ),

};

//make a system that allows args.
KD_CMD_STR g_CommandStrings[ 0xFF ] = {
	KD_DECLARE_NO_STRING,
	KD_DECLARE_STRING( L"b" ),
	KD_DECLARE_STRING( L"g" ),
	KD_DECLARE_NO_STRING,
	KD_DECLARE_NO_STRING,
	KD_DECLARE_STRING( L"!tl" ),
	KD_DECLARE_STRING( L"!ml" ),
	KD_DECLARE_STRING( L"!ctx" )

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

unsigned int wtoi( wchar_t *str1 ) {
	unsigned int r = 0, negative = 0, i = 0, base = 10;

	if ( str1[ i ] == '-' ) {
		negative = 1;
		i++;
	}

	if ( str1[ i ] == '0' && str1[ i + 1 ] == 'x' ) {
		base = 16;
		i += 2;
	}

	for ( ; str1[ i ] != 0 && ( ( str1[ i ] >= '0' && str1[ i ] <= '9' ) || ( str1[ i ] >= 'a' && str1[ i ] <= 'f' ) ); i++ ) {
		if ( ( str1[ i ] >= '0' && str1[ i ] <= '9' ) )
			r = r * base + str1[ i ] - '0';
		else
			r = r * base + ( ( str1[ i ] - 'a' ) + 10 );
	}

	if ( negative )
		r *= ( unsigned )-1;

	return r;
}

int lstrstriW(
	__in PWSTR s1,		// String.
	__in PWSTR s2	// Substring.
) {
	for ( int i = 0; s1[ i ] != 0; i++ )
		for ( int j = 0; s1[ i ] == s2[ j ]; i++, j++ )
			if ( s2[ j + 1 ] == 0 )
				return i - j;
	return -1;
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
	BaseCmd.KdCmdSize = sizeof( KD_BASE_COMMAND_SEND );

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

				if ( lstrstriW( Buffer, g_CommandStrings[ i ] ) != -1 ) {

					//KdPrint( g_CommandStrings[ i ] );

					//KdPrint( L"sent %d\n", i );

					if ( i == KD_CMD_THREAD_CONTEXT ) {

						DWORD BytesWritten;
						KD_CMDS_THREAD_CONTEXT BaseCmd;

						BaseCmd.Base.KdAckByte = KD_ACK_BYTE;
						BaseCmd.Base.KdCommandByte = KD_CMD_THREAD_CONTEXT;
						BaseCmd.Base.KdCmdSize = sizeof( KD_CMDS_THREAD_CONTEXT );

						BaseCmd.ThreadId = wtoi( Buffer + lstrlenW(g_CommandStrings[ i ]) );

						//KdPrint( L"req: %d\n", BaseCmd.ThreadId );

						WriteFile( g_KdPipe, &BaseCmd, sizeof( KD_CMDS_THREAD_CONTEXT ), &BytesWritten, NULL );
					}
					else {

						KdSendCmd( ( UCHAR )i );
					}
				}
			}
		}
	}

}
