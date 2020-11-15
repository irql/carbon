


#include <carbusr.h>

#define NTSYSCALLAPI DECLSPEC(DLLIMPORT)
#include "../kernel/nt.h"
#include "../ntgdi/ntgdi.h"

void
__C_specific_handler(

)
{

}

VOID
EntryPoint(

)
{

	UNICODE_STRING String = RTL_CONSTANT_UNICODE_STRING( L"LIME_SECURITY." );
	NtDisplayString( &String );

	NtQueryDirectoryFile( ( HANDLE )40, ( PIO_STATUS_BLOCK )41, ( PVOID )42, 43, 44, ( PUNICODE_STRING )45 );

	NtGdiDisplayString( &String );

	HANDLE ConsoleHandle;
	NtGdiCreateConsole( &ConsoleHandle, &String, 0, 80, 80 );
	NtGdiWriteConsole( ConsoleHandle, L"My ballsack is itchy.", sizeof( L"li limelime me limey." ) / 2 - 2 );

	NtGdiDisplayString( &String );

#if 0
    __try {
        __debugbreak( );
    }
    __except ( EXCEPTION_EXECUTE_HANDLER ) {

        NtDisplayString( &String );
    }
#endif

	//there is no exit function just yet lol, just let the thread return to KeExitThread, have an exception occur and have the os terminate the thread.
	//while ( 1 )
		//;

	return;
}