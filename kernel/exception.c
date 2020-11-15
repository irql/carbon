


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

    EXCEPTION_RECORD ExceptionRecord;
    ExceptionRecord.ExceptionMode = ( TrapFrame->CodeSegment & 1 ) == 0 ? KernelMode : UserMode;
    ExceptionRecord.ExceptionThread = Processor->ThreadQueue;

    //
    //  change this function to initialize some fields in the exception record
    //  for bug check parameters.
    //

    RtlpEvaluateException( 
        &ExceptionRecord, 
        TrapFrame->Interrupt );

	//
	//	two context records, we need one for unwinding and searching for the error
	//	and one for any other functions.
	//

	CONTEXT			    OriginalContext;

	PEXCEPTION_HANDLER	CatchHandler;
	PSCOPE_TABLE	    CatchScope;
	PVAD			    CatchVad;

	NTSTATUS		    ntStatus;

	RtlTrapFrameToContext( TrapFrame, &ExceptionRecord.ExceptionContext );
	RtlTrapFrameToContext( TrapFrame, &OriginalContext );

	do {

		ntStatus = RtlpFindTargetExceptionHandler(
			&ExceptionRecord,
			&CatchVad,
			&CatchHandler,
			&CatchScope );

		if ( ntStatus != STATUS_NOT_FOUND ) {

			break;
		}

		ntStatus = RtlUnwind( 
			ExceptionRecord.ExceptionThread, 
			&ExceptionRecord.ExceptionContext );

	} while ( NT_SUCCESS( ntStatus ) );
	
	//
	//	limit checks were moved to RtlUnwind, for syscalls.
	//
	
	//while ( ExceptionContext.Rsp + 0x28 < ExceptionThread->UserStackBase + ExceptionThread->UserStackSize );

	if ( CatchHandler != NULL ) {

		//
		//	this is poggers!
		//	we can jump to the exception handler.
		//

		//
		//	calls the __C_specific_handler
		//

        CatchHandler( &ExceptionRecord );

		for ( ULONG32 i = 0; i < CatchScope->Count; i++ ) {

			if (ExceptionRecord.ExceptionContext.Rip >= ( ULONG64 )CatchVad->Range.ModuleStart + CatchScope->ScopeRecord[ i ].BeginAddress &&
                ExceptionRecord.ExceptionContext.Rip <= ( ULONG64 )CatchVad->Range.ModuleStart + CatchScope->ScopeRecord[ i ].EndAddress) {

                ExceptionRecord.ExceptionContext.Rip = ( ULONG64 )CatchVad->Range.ModuleStart + CatchScope->ScopeRecord[ i ].JumpTarget;

				RtlContextToTrapFrame( TrapFrame, &ExceptionRecord.ExceptionContext );
				return;
			}
		}
	}

	switch ( ExceptionRecord.ExceptionMode ) {

	case KernelMode:

		//STATUS_UNHANDLED_SYSTEM_EXCEPTION

		KeBugCheckEx( 
            ExceptionRecord.ExceptionCode, 
            ( ULONG64 )&ExceptionRecord, 
            TrapFrame->Interrupt, 
            ( ULONG64 )TrapFrame, 
            ( ULONG64 )ExceptionRecord.ExceptionThread );

		break;
	case UserMode:

		//KeBugCheckEx( STATUS_UNHANDLED_SYSTEM_EXCEPTION, TrapFrame->Interrupt, TrapFrame->Rip, ( ULONG64 )TrapFrame, ( ULONG64 )ExceptionThread );

		KeRaiseException( ( ULONG32 )ExceptionRecord.ExceptionCode );

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

	DbgPrint( L"**EXCEPTION RAISED**\nThreadId=%.4X, Exception=%.8X Raiser=%.16P\n", KiQueryCurrentThread( )->ActiveThreadId, ExceptionCode, _ReturnAddress( ) );

	KeExitThread( );
}

