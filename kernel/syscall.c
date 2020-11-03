


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

#define DECLARE_SYSTEM_SERVICE( x, y ) { x, y }

SYSTEM_SERVICE KeServiceDescriptorTable[ ] = {
#if 0
	[ 0 ] = {0},

	[ 0 ] = { NtCreateFile, 1 },
	[ 1 ] = { NtReadFile, 1 },
	[ 2 ] = { NtWriteFile, 1 },
	[ 3 ] = { NtClose, 0 },
	[ 4 ] = { NtQueryDirectoryFile, 2 },
	[ 5 ] = { NtQueryInformationFile, 1 },
	[ 6 ] = { NtSetInformationFile, 1 },
	[ 7 ] = { NtDeviceIoControlFile, 3 }
#endif

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

#define SYSCALL_MAX ( sizeof( KeServiceDescriptorTable ) / sizeof ( SYSTEM_SERVICE ) )

VOID
KiInitializeSyscalls(

)
{
	__writemsr( IA32_MSR_EFER, __readmsr( IA32_MSR_EFER ) | 1 );

	__writemsr( IA32_MSR_LSTAR, ( unsigned long long )KiFastSystemCall );
	__writemsr( IA32_MSR_STAR, ( ( ( ULONG64 )GDT_USER_CODE64 - 16 ) << 48 ) | ( ( ( ULONG64 )GDT_KERNEL_CODE64 ) << 32 ) );
	__writemsr( IA32_MSR_SFMASK, 0 );

}








