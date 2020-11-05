


#include "driver.h"
#include "ntgdi.h"

SYSTEM_SERVICE g_NtGdiServiceDescriptorTable[ ] = {
	DECLARE_SYSTEM_SERVICE( NtGdiDisplayString, 0 )
};

NTSTATUS
NtGdiDisplayString(
	PUNICODE_STRING String
)
{
	KeProbeStringForRead( String );

	PUNICODE_STRING NewString;
	RtlAllocateAndInitUnicodeStringEx( &NewString, String->Buffer );

	NtGdiVerboseAddEntry( NewString );

	return STATUS_SUCCESS;
}

VOID
NtGdiSyscallInitialize(

)
{

	KeInstallServiceDescriptorTable( 1, sizeof( g_NtGdiServiceDescriptorTable ) / sizeof( SYSTEM_SERVICE ), g_NtGdiServiceDescriptorTable );


}