


#include <carbsup.h>

#define NTSYSCALLAPI DECLSPEC(DLLIMPORT)
#include "../kernel/nt.h"
#include "../ntgdi/ntgdi.h"

VOID
EntryPoint(

)
{

	UNICODE_STRING String = RTL_CONSTANT_UNICODE_STRING( L"LIME_SECURITY." );
	NtDisplayString( &String );

	NtQueryDirectoryFile( ( HANDLE )40, ( PIO_STATUS_BLOCK )41, ( PVOID )42, 43, 44, ( PUNICODE_STRING )45 );

	NtGdiDisplayString( &String );

	//there is no exit function just yet lol, just let the thread return to KeExitThread, have an exception occur and have the os terminate the thread.
	//while ( 1 )
		//;

	return;
}