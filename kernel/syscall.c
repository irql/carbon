


#include <carbsup.h>
#include "ki.h"
#include "nt.h"

#define IA32_MSR_EFER   0xC0000080
#define IA32_MSR_STAR   0xC0000081
#define IA32_MSR_LSTAR  0xC0000082
#define IA32_MSR_SFMASK 0xC0000084

EXTERN
VOID
KiFastSystemCall(

);

KSYSTEM_SERVICE_DESCRIPTOR_TABLE KeServiceDescriptorTable[ 16 ];

SYSTEM_SERVICE KeNativeServiceTable[ ] = {

	DECLARE_SYSTEM_SERVICE( NtCreateFile, 1 ),
	DECLARE_SYSTEM_SERVICE( NtReadFile, 1 ),
	DECLARE_SYSTEM_SERVICE( NtWriteFile, 1 ),
	DECLARE_SYSTEM_SERVICE( NtClose, 0 ),
	DECLARE_SYSTEM_SERVICE( NtQueryDirectoryFile, 2 ),
	DECLARE_SYSTEM_SERVICE( NtQueryInformationFile, 1 ),
	DECLARE_SYSTEM_SERVICE( NtSetInformationFile, 1 ),
	DECLARE_SYSTEM_SERVICE( NtDeviceIoControlFile, 3 ),
	DECLARE_SYSTEM_SERVICE( NtDisplayString, 0 ),
};

NTSTATUS
KeInstallServiceDescriptorTable(
	__in ULONG           ServiceTableIndex,
	__in ULONG			 ServiceCount,
	__in PSYSTEM_SERVICE ServiceTable
)
{

	if ( ServiceTableIndex >= 16 ) {

		return STATUS_UNSUCCESSFUL;
	}

	if ( KeServiceDescriptorTable[ ServiceTableIndex ].ServiceCount != 0 ) {

		return STATUS_UNSUCCESSFUL;
	}

	KeServiceDescriptorTable[ ServiceTableIndex ].ServiceCount = ServiceCount;
	KeServiceDescriptorTable[ ServiceTableIndex ].ServiceTable = ServiceTable;

	return STATUS_SUCCESS;
}

VOID
KiInitializeSyscalls(

)
{
	__writemsr( IA32_MSR_EFER, __readmsr( IA32_MSR_EFER ) | 1 );

	__writemsr( IA32_MSR_LSTAR, ( unsigned long long )KiFastSystemCall );
	__writemsr( IA32_MSR_STAR, ( ( ( ULONG64 )GDT_USER_CODE64 - 16 ) << 48 ) | ( ( ( ULONG64 )GDT_KERNEL_CODE64 ) << 32 ) );
	__writemsr( IA32_MSR_SFMASK, 0 );

	STATIC BOOLEAN NativeInit = FALSE;

	if ( !NativeInit ) {

		KeInstallServiceDescriptorTable( 0, sizeof( KeNativeServiceTable ) / sizeof( SYSTEM_SERVICE ), KeNativeServiceTable );

		NativeInit = TRUE;
	}

}








