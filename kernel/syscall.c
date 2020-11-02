


#include <carbsup.h>
#include "ke.h"

#define IA32_MSR_EFER   0xC0000080
#define IA32_MSR_STAR   0xC0000081
#define IA32_MSR_LSTAR  0xC0000082
#define IA32_MSR_SFMASK 0xC0000084

VOID
SyscallTest(

)
{

	PKTHREAD i = KiQueryCurrentThread( );
	printf( "syscall: %x, %x, %.16P, %.16P, %.16P\n", i->SYSCALL.CallIndex, i->SYSCALL.PreviousFlags, i->SYSCALL.PreviousIp, i->SYSCALL.PreviousStack, __readmsr( IA32_MSR_LSTAR ) );

}

#pragma warning ( disable : 4152 )

PVOID KeServiceDescriptorTable[ ] = {
	[0] = 0,
	[ 0 ] = SyscallTest,
	[ 1 ] = SyscallTest,
};

VOID
KiFastSystemCall(

);

VOID
KiInitializeSyscalls(

)
{
	__writemsr( IA32_MSR_EFER, __readmsr( IA32_MSR_EFER ) | 1 );

	__writemsr( IA32_MSR_LSTAR, ( unsigned long long )KiFastSystemCall );
	__writemsr( IA32_MSR_STAR, ( ( ( ULONG64 )GDT_USER_CODE64 - 16 ) << 48 ) | ( ( ( ULONG64 )GDT_KERNEL_CODE64 ) << 32 ) );
	__writemsr( IA32_MSR_SFMASK, 0 );

}








