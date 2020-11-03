


#include <carbsup.h>
#include "rtlp.h"
#include "ki_struct.h"
#include "pesup.h"

UCHAR RtlpUnwindOpSlotTable[ ] = {
	1,          // UWOP_PUSH_NONVOL
	2,          // UWOP_ALLOC_LARGE (or 3, special cased in lookup code)
	1,          // UWOP_ALLOC_SMALL
	1,          // UWOP_SET_FPREG
	2,          // UWOP_SAVE_NONVOL
	3,          // UWOP_SAVE_NONVOL_FAR
	0,          // UWOP_EPILOG
	0,          // UWOP_SPARE_CODE
	2,          // UWOP_SAVE_XMM128
	3,          // UWOP_SAVE_XMM128_FAR
	1           // UWOP_PUSH_MACHFRAME
};

PVAD
RtlpFindTargetModule(
	__in PKTHREAD Thread,
	__in PCONTEXT TargetContext
)
{
	PKPROCESS Process = Thread->Process;

	PVAD Vad = &Process->VadTree;

	while ( Vad != NULL ) {

		if ( TargetContext->Rip > ( ULONG64 )Vad->Range.ModuleStart &&
			TargetContext->Rip < ( ULONG64 )Vad->Range.ModuleEnd ) {

			return Vad;
		}

		Vad = Vad->Next;
	}

	return NULL;
}



//
//	unwind's a single stack frame, moving the rip to the return address and rsp to the
//	upper functions stack.
//
//	this function may destroy volatile and or non-volatile registers in the TargetContext.
//
//	it assumes that TargetContext is context, originating from Thread, and uses Thread's
//	VadTree to search for the module executing.
//

NTSTATUS
RtlUnwind(
	__in PKTHREAD Thread,
	__in PCONTEXT TargetContext
)
{
	PVOID     ModuleBase;
	PVAD      CurrentVad;

	CurrentVad = RtlpFindTargetModule( Thread, TargetContext );

	if ( CurrentVad == NULL ) {

		return STATUS_UNSUCCESSFUL;
	}

	ModuleBase = CurrentVad->Range.ModuleStart;

	PIMAGE_DOS_HEADER DosHeader = ( PIMAGE_DOS_HEADER )ModuleBase;
	PIMAGE_NT_HEADERS NtHeaders = ( PIMAGE_NT_HEADERS )( ( PCHAR )ModuleBase + DosHeader->e_lfanew );

	PIMAGE_RUNTIME_FUNCTION_ENTRY Functions = ( PIMAGE_RUNTIME_FUNCTION_ENTRY )( ( PCHAR )ModuleBase +
		NtHeaders->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXCEPTION ].VirtualAddress );
	ULONG32 FunctionCount = NtHeaders->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXCEPTION ].Size / sizeof( IMAGE_RUNTIME_FUNCTION_ENTRY );

	if ( Functions == 0 || FunctionCount == 0 ) {

		return STATUS_INVALID_PE_FILE;
	}

	for ( ULONG32 i = 0; i < FunctionCount; i++ ) {

		if ( TargetContext->Rip >= ( ( ULONG64 )ModuleBase + Functions[ i ].BeginAddress ) &&
			TargetContext->Rip <= ( ( ULONG64 )ModuleBase + Functions[ i ].EndAddress ) ) {

			return RtlpUnwindPrologue(
				Thread,
				TargetContext,
				CurrentVad->Range.ModuleStart,
				&Functions[ i ] );
		}
	}

	return STATUS_UNSUCCESSFUL;
}

NTSTATUS
RtlpUnwindPrologue(
	__in PKTHREAD Thread,
	__in PCONTEXT TargetContext,
	__in PVOID    TargetVadBase,
	__in PIMAGE_RUNTIME_FUNCTION_ENTRY FunctionEntry
)
{
	PULONG64  IntegerRegister;

	IntegerRegister = &TargetContext->Rax;

	PUNWIND_INFO UnwindInfo = ( PUNWIND_INFO )( ( PCHAR )TargetVadBase + FunctionEntry->UnwindData );

	for ( ULONG32 j = 0; j < UnwindInfo->CountOfCodes; ) {

		UCHAR OpInfo = UnwindInfo->UnwindCode[ j ].OpInfo;
		UCHAR UnwindOp = UnwindInfo->UnwindCode[ j ].UnwindOp;

		ULONG32 FrameOffset;

		if ( ( ULONG64 )UnwindInfo->UnwindCode[ j ].CodeOffset > ( TargetContext->Rip - ( ( ULONG64 )TargetVadBase + FunctionEntry->BeginAddress ) ) ) {

			j += RtlpUnwindOpSlotTable[ UnwindOp ];

			switch ( UnwindOp ) {

			case UWOP_ALLOC_LARGE:
				if ( OpInfo != 0 ) {

					j++;
				}
				break;

			default:
				break;
			}


		}
		else {

			switch ( UnwindOp ) {

			case UWOP_ALLOC_SMALL:
				TargetContext->Rsp += ( OpInfo * 8 ) + 8;
				break;

			case UWOP_PUSH_NONVOL:
				IntegerRegister[ OpInfo ] = *( PULONG64 )TargetContext->Rsp;
				TargetContext->Rsp += 8;
				break;

			case UWOP_SET_FPREG:
				TargetContext->Rsp = IntegerRegister[ UnwindInfo->FrameRegister ];
				TargetContext->Rsp -= UnwindInfo->FrameOffset * 16;
				break;

			case UWOP_ALLOC_LARGE:
				j++;
				FrameOffset = UnwindInfo->UnwindCode[ j ].FrameOffset;

				if ( OpInfo != 0 ) {
					j++;
					FrameOffset += ( UnwindInfo->UnwindCode[ j ].FrameOffset << 16 );
				}
				else {

					FrameOffset *= 8;
				}

				TargetContext->Rsp += FrameOffset;
				break;

			case UWOP_SAVE_NONVOL:
				j++;
				FrameOffset = UnwindInfo->UnwindCode[ j ].FrameOffset * 8;
				IntegerRegister[ OpInfo ] = *( PULONG64 )( Thread->UserStackBase + FrameOffset );
				break;

			case UWOP_SAVE_NONVOL_FAR:
				j += 2;
				FrameOffset = UnwindInfo->UnwindCode[ j - 1 ].FrameOffset;
				FrameOffset += ( UnwindInfo->UnwindCode[ j ].FrameOffset << 16 );

				IntegerRegister[ OpInfo ] = *( PULONG64 )( Thread->UserStackBase + FrameOffset );
				break;

			case UWOP_SAVE_XMM128:
				j++;
				FrameOffset = UnwindInfo->UnwindCode[ j ].FrameOffset * 16;
				//unimpl
				break;

			case UWOP_SAVE_XMM128_FAR:
				//unimpl.
				break;

			case UWOP_PUSH_MACHFRAME:

				break;

			default:
				//hm.
				break;
			}

			j++;
		}
	}

	//
	//	if code got here, we either have to do more, chained, unwinding or
	//	we're done, and the return address is ready in rsp.
	//

	if ( ( UnwindInfo->Flags & UNW_FLAG_CHAININFO ) != 0 ) {


		return RtlpUnwindPrologue(
			Thread,
			TargetContext,
			TargetVadBase,
			( PIMAGE_RUNTIME_FUNCTION_ENTRY )( &UnwindInfo->UnwindCode[ UnwindInfo->CountOfCodes + ( UnwindInfo->CountOfCodes & 1 ) ] ) );
	}
	else {

		TargetContext->Rip = *( PULONG64 )TargetContext->Rsp;
		TargetContext->Rsp += 8;

		return STATUS_SUCCESS;
	}

}