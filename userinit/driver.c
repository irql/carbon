


#include <carbsup.h>

#define NTSYSCALLAPI DECLSPEC(DLLIMPORT)
#include "../kernel/nt.h"

VOID
EntryPoint(

)
{

	UNICODE_STRING String = RTL_CONSTANT_UNICODE_STRING( L"LIME_SECURITY." );
	NtDisplayString( &String );

	while ( 1 )
		;
}