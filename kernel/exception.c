


#include <carbsup.h>
#include "ki.h"
#include "ki_struct.h"
#include "rtlp.h"

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

	PKTHREAD		ExceptionThread = Processor->ThreadQueue;

	//
	//	two context records, we need one for unwinding and searching for the error
	//	and one for any other functions.
	//

	CONTEXT			ExceptionContext;
	CONTEXT			OriginalContext;

	PVOID			ExceptionHandler;
	PSCOPE_TABLE	ExceptionScope;
	PVAD			ExceptionVad;

	NTSTATUS		ntStatus;

	TRAPFRAME_TO_CONTEXT( TrapFrame, &ExceptionContext );
	TRAPFRAME_TO_CONTEXT( TrapFrame, &OriginalContext );

	do {

		ntStatus = RtlpFindTargetExceptionHandler(
			ExceptionThread,
			&ExceptionContext,
			&ExceptionVad,
			&ExceptionHandler,
			&ExceptionScope );

		if ( ntStatus != STATUS_NOT_FOUND ) {

			break;
		}

		ntStatus = RtlUnwind( 
			ExceptionThread, 
			&ExceptionContext );

		if ( !NT_SUCCESS( ntStatus ) ) {

			break;
		}

	} while ( ExceptionContext.Rsp + 0x28 < ExceptionThread->UserStackBase + ExceptionThread->UserStackSize );

	if ( ExceptionHandler != NULL ) {

		//
		//	this is poggers!
		//	we can jump to the exception handler.
		//

		//
		//	calls the __C_specific_handler
		//

		( ( void( *)( ) )ExceptionHandler )( );

		for ( ULONG32 i = 0; i < ExceptionScope->Count; i++ ) {

			if ( ExceptionContext.Rip >= ( ULONG64 )ExceptionVad->Range.ModuleStart + ExceptionScope->ScopeRecord[ i ].BeginAddress &&
				 ExceptionContext.Rip <= ( ULONG64 )ExceptionVad->Range.ModuleStart + ExceptionScope->ScopeRecord[ i ].EndAddress ) {

				ExceptionContext.Rip = ( ULONG64 )ExceptionVad->Range.ModuleStart + ExceptionScope->ScopeRecord[ i ].JumpTarget;

				CONTEXT_TO_TRAPFRAME( TrapFrame, &ExceptionContext );
				return;
			}
		}
	}

	switch ( ExceptionMode ) {

	case KernelMode:

		//STATUS_UNHANDLED_SYSTEM_EXCEPTION

		KeBugCheckEx( STATUS_UNHANDLED_SYSTEM_EXCEPTION, TrapFrame->Interrupt, TrapFrame->Rip, ( ULONG64 )TrapFrame, ( ULONG64 )ExceptionThread );

		break;
	case UserMode:

		//KeBugCheckEx( STATUS_UNHANDLED_SYSTEM_EXCEPTION, TrapFrame->Interrupt, TrapFrame->Rip, ( ULONG64 )TrapFrame, ( ULONG64 )ExceptionThread );

		KeRaiseException( ( ULONG32 )STATUS_ACCESS_VIOLATION );

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

