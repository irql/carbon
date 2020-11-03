


#include <carbsup.h>
#include "ki.h"
#include "ki_struct.h"

VOID
KiExceptionTrap(
	__inout PKTRAP_FRAME TrapFrame,
	__in    PKPCR        Processor
)
{
	Processor;

	//
	//	Lime Handling.
	//

	//
	//	uses the cs rpl to test where it occurred, this means if it happens
	//	during a syscall, it happens in kernel mode (and is unhandled, bugcheck)
	//

	KPROCESSOR_MODE ExceptionMode = ( TrapFrame->CodeSegment & 1 ) == 0 ? KernelMode : UserMode;

	PKTHREAD ExceptionThread = Processor->ThreadQueue;

	//
	//	there should really be more checks to try figure out what happened.
	//

	switch ( ExceptionMode ) {

	case KernelMode:

		//STATUS_UNHANDLED_SYSTEM_EXCEPTION

		KeBugCheckEx( STATUS_UNHANDLED_SYSTEM_EXCEPTION, TrapFrame->Interrupt, TrapFrame->Rip, ( ULONG64 )TrapFrame, ( ULONG64 )ExceptionThread );

		break;
	case UserMode:

		KeBugCheckEx( STATUS_UNHANDLED_SYSTEM_EXCEPTION, TrapFrame->Interrupt, TrapFrame->Rip, ( ULONG64 )TrapFrame, ( ULONG64 )ExceptionThread );

		//KeRaiseException( ( ULONG32 )STATUS_ACCESS_VIOLATION );

		break;
	}
}

DECLSPEC( noreturn )
VOID
KeRaiseException(
	__in ULONG32 ExceptionCode
)
{
	//
	//	ok so, this routine should be called from a function which has been
	//	called from user mode and something has went wrong.
	//	it should terminate the thread and "handle" the exception.
	//

	DbgPrint( L"**EXCEPTION RAISED**\n %#.8X %#.8X\n", KiQueryCurrentThread( )->ActiveThreadId, ExceptionCode );

	KeExitThread( );
}

