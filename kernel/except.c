


#include <carbsup.h>
#include "rtlp.h"
#include "ki_struct.h"
#include "pesup.h"

VOID
__C_specific_handler(

)
{

	printf( "__C_specific_handler executed.\n" );

	return;
}

NTSTATUS
RtlpFindTargetExceptionHandler(
	__in    PKTHREAD      Thread,
	__in    PCONTEXT      ExceptionContext,
	__inout PVAD*         ExceptionVad,
	__inout PVOID*        ExceptionHandler,
	__inout PSCOPE_TABLE* ExceptionScope
)
{
	PVOID     ModuleBase;

	*ExceptionHandler = NULL;

	*ExceptionVad = RtlpFindTargetModule( Thread, ExceptionContext );

	if ( *ExceptionVad == NULL ) {

		return STATUS_UNSUCCESSFUL;
	}

	ModuleBase = ( *ExceptionVad )->Range.ModuleStart;

	PIMAGE_DOS_HEADER DosHeader = ( PIMAGE_DOS_HEADER )ModuleBase;
	PIMAGE_NT_HEADERS NtHeaders = ( PIMAGE_NT_HEADERS )( ( PCHAR )ModuleBase + DosHeader->e_lfanew );

	PIMAGE_RUNTIME_FUNCTION_ENTRY FunctionEntry = ( PIMAGE_RUNTIME_FUNCTION_ENTRY )( ( PCHAR )ModuleBase +
		NtHeaders->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXCEPTION ].VirtualAddress );
	ULONG32 FunctionCount = NtHeaders->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXCEPTION ].Size / sizeof( IMAGE_RUNTIME_FUNCTION_ENTRY );

	if ( FunctionEntry == 0 || FunctionCount == 0 ) {

		return STATUS_INVALID_PE_FILE;
	}

	for ( ULONG32 i = 0; i < FunctionCount; i++ ) {

		if ( ExceptionContext->Rip >= ( ( ULONG64 )ModuleBase + FunctionEntry[ i ].BeginAddress ) &&
			ExceptionContext->Rip <= ( ( ULONG64 )ModuleBase + FunctionEntry[ i ].EndAddress ) ) {

			PUNWIND_INFO UnwindInfo = ( PUNWIND_INFO )( ( PCHAR )ModuleBase + FunctionEntry[ i ].UnwindData );

			if ( UnwindInfo->Flags & UNW_FLAG_EHANDLER ) {

				*ExceptionHandler = ( PVOID )( ( ULONG64 )ModuleBase + 
					*( ULONG* )( ( PCHAR )UnwindInfo + 4 + sizeof( UNWIND_CODE ) * UnwindInfo->CountOfCodes ) );
				*ExceptionScope = ( PSCOPE_TABLE )( ( PCHAR )UnwindInfo + 8 + sizeof( UNWIND_CODE ) * UnwindInfo->CountOfCodes );
				return STATUS_SUCCESS;
			}

		}
	}

	return STATUS_NOT_FOUND;
}

