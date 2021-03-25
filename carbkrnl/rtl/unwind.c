


#include <carbsup.h>
#include "rtlp.h"
#include "../mm/mi.h"
#include "../hal/halp.h"
#include "../ke/ki.h"

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

//
// Unwinds the stack, going up a frame, as if you were at
// offset 0 of a function (return on the top of the stack).
//
// This function may destory volatile and/or non-volatile registers
// inside the TargetContext.
//
// The thread parameter is only used for the Process member,
// stack base, stack length and potentially for KiFastSystemCall64 unwinding.
//

NTSTATUS
RtlUnwindFrame(
    _In_ PKTHREAD        Thread,
    _In_ PCONTEXT        TargetContext,
    _In_ ULONG64         StackBase,
    _In_ ULONG64         StackLength
)
{
    StackBase;
    StackLength;

    NTSTATUS ntStatus;
    PVOID TargetBase;
    PMM_VAD CurrentVad;
    PIMAGE_DOS_HEADER HeaderDos;
    PIMAGE_NT_HEADERS HeadersNt;
    PIMAGE_RUNTIME_FUNCTION FunctionTable;
    ULONG64 FunctionCount;
    ULONG64 CurrentFunction;
    ULONG64 FrameBase;

    CurrentVad = MiFindVadByAddress( Thread->Process, TargetContext->Rip );

    if ( CurrentVad == NULL ) {

        //
        // Setup a system so that if an instruction
        // pointer is not found within a function, it should
        // just repeat this process over and over until either
        // hitting the stack limit, or hitting a return address
        // for a function which could be inside a function table.
        //

        TargetContext->Rip = *( PULONG64 )TargetContext->Rsp;
        TargetContext->Rsp += 8;
        return STATUS_SUCCESS;
    }

    TargetBase = ( PVOID )CurrentVad->Start;
    HeaderDos = ( PIMAGE_DOS_HEADER )( TargetBase );
    HeadersNt = ( PIMAGE_NT_HEADERS )( ( PUCHAR )TargetBase + HeaderDos->e_lfanew );
    FunctionTable = ( PIMAGE_RUNTIME_FUNCTION )( ( PUCHAR )TargetBase +
                                                 HeadersNt->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXCEPTION ].VirtualAddress );
    FunctionCount = HeadersNt->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXCEPTION ].Size / sizeof( IMAGE_RUNTIME_FUNCTION );

    if ( FunctionTable == TargetBase ||
         FunctionCount == 0 ) {

        return STATUS_INVALID_IMAGE;
    }

    for ( CurrentFunction = 0; CurrentFunction < FunctionCount; CurrentFunction++ ) {

        if ( TargetContext->Rip >= ( ULONG64 )TargetBase + FunctionTable[ CurrentFunction ].BeginAddress &&
             TargetContext->Rip < ( ULONG64 )TargetBase + FunctionTable[ CurrentFunction ].EndAddress ) {

            //
            // I think windows unwinds by instruction instead of like this?
            // at least it should work in most cases
            //

            FrameBase = TargetContext->Rsp;

            ntStatus = RtlpUnwindPrologueFrame( Thread,
                                                TargetContext,
                                                TargetBase,
                                                &FunctionTable[ CurrentFunction ],
                                                StackBase,
                                                StackLength,
                                                &FrameBase );

            if ( !NT_SUCCESS( ntStatus ) ) {

                return ntStatus;
            }

            return RtlpUnwindPrologue( Thread,
                                       TargetContext,
                                       TargetBase,
                                       &FunctionTable[ CurrentFunction ],
                                       FrameBase );
        }
    }

    TargetContext->Rip = *( PULONG64 )TargetContext->Rsp;
    TargetContext->Rsp += 8;
    return STATUS_SUCCESS;
}

NTSTATUS
RtlpUnwindPrologueFrame(
    _In_  PKTHREAD                Thread,
    _In_  PCONTEXT                TargetContext,
    _In_  PVOID                   TargetBase,
    _In_  PIMAGE_RUNTIME_FUNCTION TargetFunction,
    _In_  ULONG64                 StackBase,
    _In_  ULONG64                 StackLength,
    _Out_ PULONG64                FrameBase
)
{
    PULONG64 GeneralRegisters;
    PUNWIND_INFO UnwindInfo;
    ULONG64 CurrentCode;
    UCHAR OpInfo;
    UCHAR UnwindOp;
    ULONG32 FrameOffset;

    GeneralRegisters = &TargetContext->Rax;
    UnwindInfo = ( PUNWIND_INFO )( ( PUCHAR )TargetBase + TargetFunction->UnwindData );

    for ( CurrentCode = 0; CurrentCode < UnwindInfo->CountOfCodes; ) {
        OpInfo = UnwindInfo->UnwindCode[ CurrentCode ].OpInfo;
        UnwindOp = UnwindInfo->UnwindCode[ CurrentCode ].UnwindOp;

        if ( ( ULONG64 )UnwindInfo->UnwindCode[ CurrentCode ].CodeOffset >
            ( TargetContext->Rip - ( ULONG64 )TargetBase + TargetFunction->BeginAddress ) ) {
            CurrentCode += RtlpUnwindOpSlotTable[ UnwindOp ];

            switch ( UnwindOp ) {
            case UWOP_ALLOC_LARGE:;
                if ( OpInfo != 0 ) {

                    CurrentCode++;
                }
                break;
            default:
                break;
            }

        }
        else {
            switch ( UnwindOp ) {

            case UWOP_ALLOC_SMALL:;
                *FrameBase += ( ( ULONG64 )OpInfo * 8 ) + 8;
                break;

            case UWOP_PUSH_NONVOL:;
                *FrameBase += 8;
                break;

            case UWOP_SET_FPREG:;
                *FrameBase = GeneralRegisters[ UnwindInfo->FrameRegister ];
                *FrameBase -= UnwindInfo->FrameOffset * 16;
                break;

            case UWOP_ALLOC_LARGE:;
                CurrentCode++;
                FrameOffset = UnwindInfo->UnwindCode[ CurrentCode ].FrameOffset;

                if ( OpInfo != 0 ) {
                    CurrentCode++;
                    FrameOffset += ( ULONG32 )UnwindInfo->UnwindCode[ CurrentCode ].FrameOffset << 16;
                }
                else {

                    FrameOffset *= 8;
                }

                *FrameBase += FrameOffset;
                break;

            case UWOP_SAVE_NONVOL:;
                CurrentCode++;
                break;

            case UWOP_SAVE_NONVOL_FAR:;
                CurrentCode += 2;
                FrameOffset = UnwindInfo->UnwindCode[ CurrentCode - 1 ].FrameOffset;
                FrameOffset += ( ULONG32 )UnwindInfo->UnwindCode[ CurrentCode ].FrameOffset << 16;

                //GeneralRegisters[ OpInfo ] = *( PULONG64 )( Thread->StackBase + FrameOffset );
                break;

            case UWOP_SAVE_XMM128:;
                CurrentCode++;
                FrameOffset = ( ULONG32 )UnwindInfo->UnwindCode[ CurrentCode ].FrameOffset * 16;
                //unimpl
                break;

            case UWOP_SAVE_XMM128_FAR:;
                //unimpl.
                break;

            case UWOP_PUSH_MACHFRAME:;

                if ( OpInfo != 0 ) { // is there an error code pushed?

                    //TargetContext->Rip = *( PULONG64 )( TargetContext->Rsp + 8 );
                    //TargetContext->Rsp = *( PULONG64 )( TargetContext->Rsp + 32 );
                }
                else {

                    //TargetContext->Rip = *( PULONG64 )( TargetContext->Rsp );
                    //TargetContext->Rsp = *( PULONG64 )( TargetContext->Rsp + 24 );
                }
                break;

            default:
                //hm.
                break;
            }

            CurrentCode++;
        }
    }

    //
    //  If code got here, we either have to do more, chained, unwinding or
    //  we're done, and the return address is ready in rsp.
    //

    if ( UnwindInfo->Flags & UNW_FLAG_CHAININFO ) {

        return RtlpUnwindPrologueFrame( Thread,
                                        TargetContext,
                                        TargetBase,
                                        ( PIMAGE_RUNTIME_FUNCTION )( &UnwindInfo->UnwindCode[ UnwindInfo->CountOfCodes + ( UnwindInfo->CountOfCodes & 1 ) ] ),
                                        StackBase,
                                        StackLength,
                                        FrameBase );
    }
    else {

        return STATUS_SUCCESS;
    }
}

NTSTATUS
RtlpUnwindPrologue(
    _In_ PKTHREAD                Thread,
    _In_ PCONTEXT                TargetContext,
    _In_ PVOID                   TargetBase,
    _In_ PIMAGE_RUNTIME_FUNCTION TargetFunction,
    _In_ ULONG64                 FrameBase
)
{
    PULONG64 GeneralRegisters;
    PUNWIND_INFO UnwindInfo;
    ULONG64 CurrentCode;
    UCHAR OpInfo;
    UCHAR UnwindOp;
    ULONG32 FrameOffset;

    //
    // GeneralRegisters should be an array of
    // registers ordered rax -> r15
    //

    GeneralRegisters = &TargetContext->Rax;
    UnwindInfo = ( PUNWIND_INFO )( ( PUCHAR )TargetBase + TargetFunction->UnwindData );

    for ( CurrentCode = 0; CurrentCode < UnwindInfo->CountOfCodes; ) {
        OpInfo = UnwindInfo->UnwindCode[ CurrentCode ].OpInfo;
        UnwindOp = UnwindInfo->UnwindCode[ CurrentCode ].UnwindOp;

        if ( ( ULONG64 )UnwindInfo->UnwindCode[ CurrentCode ].CodeOffset >
            ( TargetContext->Rip - ( ULONG64 )TargetBase + TargetFunction->BeginAddress ) ) {
            CurrentCode += RtlpUnwindOpSlotTable[ UnwindOp ];

            switch ( UnwindOp ) {
            case UWOP_ALLOC_LARGE:;
                if ( OpInfo != 0 ) {

                    CurrentCode++;
                }
                break;
            default:
                break;
            }

        }
        else {
            switch ( UnwindOp ) {

            case UWOP_ALLOC_SMALL:;
                TargetContext->Rsp += ( ( ULONG64 )OpInfo * 8 ) + 8;
                break;

            case UWOP_PUSH_NONVOL:;
                GeneralRegisters[ OpInfo ] = *( PULONG64 )TargetContext->Rsp;
                TargetContext->Rsp += 8;
                break;

            case UWOP_SET_FPREG:;
                TargetContext->Rsp = GeneralRegisters[ UnwindInfo->FrameRegister ];
                TargetContext->Rsp -= UnwindInfo->FrameOffset * 16;
                break;

            case UWOP_ALLOC_LARGE:;
                CurrentCode++;
                FrameOffset = UnwindInfo->UnwindCode[ CurrentCode ].FrameOffset;

                if ( OpInfo != 0 ) {
                    CurrentCode++;
                    FrameOffset += ( ULONG32 )UnwindInfo->UnwindCode[ CurrentCode ].FrameOffset << 16;
                }
                else {

                    FrameOffset *= 8;
                }

                TargetContext->Rsp += FrameOffset;
                break;

            case UWOP_SAVE_NONVOL:; // be careful of the stack base. TODO: should that be stack base + stack length?
                CurrentCode++;
                FrameOffset = ( ULONG32 )UnwindInfo->UnwindCode[ CurrentCode ].FrameOffset * 8;
                GeneralRegisters[ OpInfo ] = *( PULONG64 )( FrameBase + FrameOffset );
                break;

            case UWOP_SAVE_NONVOL_FAR:;
                CurrentCode += 2;
                FrameOffset = UnwindInfo->UnwindCode[ CurrentCode - 1 ].FrameOffset;
                FrameOffset += ( ULONG32 )UnwindInfo->UnwindCode[ CurrentCode ].FrameOffset << 16;

                GeneralRegisters[ OpInfo ] = *( PULONG64 )( Thread->StackBase + FrameOffset );
                break;

            case UWOP_SAVE_XMM128:;
                CurrentCode++;
                FrameOffset = ( ULONG32 )UnwindInfo->UnwindCode[ CurrentCode ].FrameOffset * 16;
                //unimpl
                break;

            case UWOP_SAVE_XMM128_FAR:;
                //unimpl.
                break;

            case UWOP_PUSH_MACHFRAME:;

                if ( OpInfo != 0 ) { // is there an error code pushed?

                    TargetContext->Rip = *( PULONG64 )( TargetContext->Rsp + 8 );
                    TargetContext->Rsp = *( PULONG64 )( TargetContext->Rsp + 32 );
                }
                else {

                    TargetContext->Rip = *( PULONG64 )( TargetContext->Rsp );
                    TargetContext->Rsp = *( PULONG64 )( TargetContext->Rsp + 24 );
                }
                break;

            default:
                //hm.
                break;
            }

            CurrentCode++;
        }
    }

    //
    //  If code got here, we either have to do more, chained, unwinding or
    //  we're done, and the return address is ready in rsp.
    //

    if ( UnwindInfo->Flags & UNW_FLAG_CHAININFO ) {

        return RtlpUnwindPrologue( Thread,
                                   TargetContext,
                                   TargetBase,
                                   ( PIMAGE_RUNTIME_FUNCTION )( &UnwindInfo->UnwindCode[ UnwindInfo->CountOfCodes + ( UnwindInfo->CountOfCodes & 1 ) ] ),
                                   FrameBase );
    }
    else {

        TargetContext->Rip = *( PULONG64 )( TargetContext->Rsp );
        TargetContext->Rsp += 8;
        return STATUS_SUCCESS;
    }
}

NTSTATUS
RtlpFindExceptionHandler(
    _In_  PEXCEPTION_RECORD   Record,
    _In_  PMM_VAD             Vad,
    _Out_ PEXCEPTION_HANDLER* Handler,
    _Out_ PUNWIND_INFO*       Unwind
)
{
    ULONG64 ModuleBase;
    PIMAGE_DOS_HEADER HeaderDos;
    PIMAGE_NT_HEADERS HeadersNt;
    PIMAGE_RUNTIME_FUNCTION FunctionTable;
    ULONG64 FunctionCount;
    ULONG64 CurrentFunction;
    PUNWIND_INFO UnwindInfo;

    *Handler = NULL;
    *Unwind = NULL;

    ModuleBase = Vad->Start;
    HeaderDos = ( PIMAGE_DOS_HEADER )( ModuleBase );
    HeadersNt = ( PIMAGE_NT_HEADERS )( ModuleBase + HeaderDos->e_lfanew );
    FunctionTable = ( PIMAGE_RUNTIME_FUNCTION )( ModuleBase +
                                                 HeadersNt->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXCEPTION ].VirtualAddress );
    FunctionCount = HeadersNt->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXCEPTION ].Size / sizeof( IMAGE_RUNTIME_FUNCTION );

    if ( ( ULONG64 )FunctionTable == ModuleBase || FunctionCount == 0 ) {

        return STATUS_INVALID_IMAGE;
    }

    for ( CurrentFunction = 0;
          CurrentFunction < FunctionCount;
          CurrentFunction++ ) {

        if ( Record->ExceptionContext.Rip >= ModuleBase + FunctionTable[ CurrentFunction ].BeginAddress &&
             Record->ExceptionContext.Rip < ModuleBase + FunctionTable[ CurrentFunction ].EndAddress ) {

            UnwindInfo = ( PUNWIND_INFO )( ModuleBase + FunctionTable[ CurrentFunction ].UnwindData );

            if ( UnwindInfo->Flags & UNW_FLAG_EHANDLER ) {
#if 1
#if 1
                * Handler = ( PVOID )( ModuleBase + *( ULONG32* )( ( ULONG64 )UnwindInfo + 4 +
                                                                   sizeof( UNWIND_CODE ) * UnwindInfo->CountOfCodes +
                                                                   ( sizeof( UNWIND_CODE ) * UnwindInfo->CountOfCodes ) % 4 ) );
#endif
                //*Handler = ( PVOID )( ModuleBase + *( ULONG32* )( ( ULONG64 )UnwindInfo + UnwindInfo->CountOfCodes * sizeof( UNWIND_CODE ) ) );
                *Unwind = UnwindInfo;

                return STATUS_SUCCESS;
#else
                /*
                CurrentScope = ( PVOID )( ( ULONG64 )UnwindInfo + 8 +
                                          sizeof( UNWIND_CODE ) * UnwindInfo->CountOfCodes +
                                          ( sizeof( UNWIND_CODE ) * UnwindInfo->CountOfCodes ) % 4 );
                */
                CurrentScope = ( PVOID )( ModuleBase + *( ULONG32* )( ( ULONG64 )UnwindInfo + UnwindInfo->CountOfCodes * sizeof( UNWIND_CODE ) ) + 8 );

                RtlDebugPrint( L"Brual: %ull %d\n", ( PCHAR )CurrentScope - ModuleBase, CurrentScope->Count );

                for ( CurrentRecord = 0; CurrentRecord < CurrentScope->Count; CurrentRecord++ ) {

                    if ( Record->ExceptionContext.Rip >= ModuleBase + CurrentScope->ScopeRecord[ CurrentRecord ].BeginAddress &&
                         Record->ExceptionContext.Rip < ModuleBase + CurrentScope->ScopeRecord[ CurrentRecord ].EndAddress ) {

                        RtlDebugPrint( L"found a pogger\n" );

                        *Handler = ( PVOID )( ModuleBase + *( ULONG32* )( ( ULONG64 )UnwindInfo + UnwindInfo->CountOfCodes * sizeof( UNWIND_CODE ) ) );
                        //*Scope = ( PVOID )( ModuleBase + *( ULONG32* )( ( ULONG64 )UnwindInfo + UnwindInfo->CountOfCodes * sizeof( UNWIND_CODE ) ) + 8 );
                        /**Handler = ( PVOID )( ModuleBase + *( ULONG32* )( ( ULONG64 )UnwindInfo + 4 +
                                                                           sizeof( UNWIND_CODE ) * UnwindInfo->CountOfCodes +
                                                                           ( sizeof( UNWIND_CODE ) * UnwindInfo->CountOfCodes ) % 4 ) );*/
                        *Scope = CurrentScope;

                        if ( UnwindInfo->FrameRegister != 0 ) {

                            GeneralRegisters[ UnwindInfo->FrameRegister ] = Record->ExceptionContext.Rsp + 16 * UnwindInfo->FrameOffset;
                        }

                        return STATUS_SUCCESS;
                    }
                }

                return STATUS_NOT_FOUND;
#endif          
            }
        }
    }

    return STATUS_NOT_FOUND;
}
