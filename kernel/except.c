


#include <carbsup.h>
#include "rtlp.h"
#include "ki_struct.h"
#include "pesup.h"

VOID
__C_specific_handler(
    __in PEXCEPTION_RECORD ExceptionRecord
)
{
    ExceptionRecord;

	printf( "__C_specific_handler executed.\n" );

	return;
}

VOID
RtlpEvaluateException(
    __in PEXCEPTION_RECORD  ExceptionRecord,
    __in ULONG64            ExceptionInterrupt
)
{

    NT_ASSERT( ExceptionInterrupt < 32 );

    switch ( ExceptionInterrupt ) {
    case E_PF:
        ExceptionRecord->ExceptionCode = EXCEPTION_ACCESS_VIOLATION;
        ExceptionRecord->ExceptionSeverity = ExceptionNormal;
        break;
    case E_GP:
    case E_UD:
        ExceptionRecord->ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
        ExceptionRecord->ExceptionSeverity = ExceptionNormal;

        break;
    case E_BP:
        ExceptionRecord->ExceptionCode = EXCEPTION_BREAKPOINT;
        ExceptionRecord->ExceptionSeverity = ExceptionIgnore;

        break;
    case E_DF:
        ExceptionRecord->ExceptionCode = EXCEPTION_DOUBLE_FAULT;
        ExceptionRecord->ExceptionSeverity = ExceptionFatal;

        break;
    default:
        ExceptionRecord->ExceptionCode = EXCEPTION_ACCESS_VIOLATION;
        ExceptionRecord->ExceptionSeverity = ExceptionNormal;

        break;
    }

    
}


NTSTATUS
RtlpFindTargetExceptionHandler(
    __in    PEXCEPTION_RECORD   ExceptionRecord,
    __inout PVAD*               CatchVad,
    __inout PEXCEPTION_HANDLER* CatchHandler,
    __inout PSCOPE_TABLE*       CatchScope
)
{
	PVOID     ModuleBase;

	*CatchHandler = NULL;

	*CatchVad = RtlpFindTargetModule( ExceptionRecord->ExceptionThread, &ExceptionRecord->ExceptionContext );

	if ( *CatchVad == NULL ) {

		return STATUS_UNSUCCESSFUL;
	}

	ModuleBase = ( *CatchVad )->Range.ModuleStart;

	PIMAGE_DOS_HEADER DosHeader = ( PIMAGE_DOS_HEADER )ModuleBase;
	PIMAGE_NT_HEADERS NtHeaders = ( PIMAGE_NT_HEADERS )( ( PCHAR )ModuleBase + DosHeader->e_lfanew );

	PIMAGE_RUNTIME_FUNCTION_ENTRY FunctionEntry = ( PIMAGE_RUNTIME_FUNCTION_ENTRY )( ( PCHAR )ModuleBase +
		NtHeaders->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXCEPTION ].VirtualAddress );
	ULONG32 FunctionCount = NtHeaders->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXCEPTION ].Size / sizeof( IMAGE_RUNTIME_FUNCTION_ENTRY );

	if ( FunctionEntry == 0 || FunctionCount == 0 ) {

		return STATUS_INVALID_PE_FILE;
	}

	for ( ULONG32 i = 0; i < FunctionCount; i++ ) {

		if ( ExceptionRecord->ExceptionContext.Rip >= ( ( ULONG64 )ModuleBase + FunctionEntry[ i ].BeginAddress ) &&
             ExceptionRecord->ExceptionContext.Rip <= ( ( ULONG64 )ModuleBase + FunctionEntry[ i ].EndAddress ) ) {

			PUNWIND_INFO UnwindInfo = ( PUNWIND_INFO )( ( PCHAR )ModuleBase + FunctionEntry[ i ].UnwindData );

			if ( UnwindInfo->Flags & UNW_FLAG_EHANDLER ) {

				*CatchHandler = ( PVOID )( ( ULONG64 )ModuleBase + 
					*( ULONG* )( ( PCHAR )UnwindInfo + 4 + sizeof( UNWIND_CODE ) * UnwindInfo->CountOfCodes ) );
				*CatchScope = ( PSCOPE_TABLE )( ( PCHAR )UnwindInfo + 8 + sizeof( UNWIND_CODE ) * UnwindInfo->CountOfCodes );
				return STATUS_SUCCESS;
			}

		}
	}

	return STATUS_NOT_FOUND;
}

