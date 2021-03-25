


//
// This file is almost completely pasted from 
// carbkrnl.vcxproj, it has been changed slightly
// to work with KdpReadDebuggee.
//

#include "carbon.h"
#include "kd.h"
#include "kdp.h"
#include "pdb.h"
#include "osl/osl.h"

NTSTATUS
KdpUnwindPrologue(
    _In_ PKTHREAD                Thread,
    _In_ PCONTEXT                TargetContext,
    _In_ PVOID                   TargetBase,
    _In_ PIMAGE_RUNTIME_FUNCTION TargetFunction
);

UCHAR KdpUnwindOpSlotTable[ ] = {
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
KdpUnwindFrame(
    _In_ PKTHREAD Thread,
    _In_ PCONTEXT TargetContext
)
{
    STATIC ULONG64 KiFastSystemCall = 0;
    STATIC LONG    SystemCallOffset = 0;
    NTSTATUS ntStatus;
    PIMAGE_RUNTIME_FUNCTION FunctionTable;
    ULONG64 FunctionCount;
    ULONG64 CurrentFunction;
    ULONG64 StackBase;
    ULONG64 StackLength;
    ULONG64 KernelStackBase;
    ULONG64 KernelStackLength;
    PKD_CACHED_MODULE Cached;
#pragma pack(push, 1)
    struct {
        ULONG32 ServiceNumber;
        ULONG32 PreviousEFlags;
        ULONG64 PreviousIp;
        ULONG64 PreviousStack;
    } syscall;
#pragma pack(pop)

    //KdpGetThreadStack( Thread, &StackBase, &StackLength );
    KdpGetThreadStackEx( Thread, &StackBase, &StackLength, &KernelStackBase, &KernelStackLength );

    if ( ( TargetContext->Rsp + 0x28 >= StackBase + StackLength ||
           TargetContext->Rsp < StackBase ) &&
           ( TargetContext->Rsp + 0x28 >= KernelStackBase + KernelStackLength ||
             TargetContext->Rsp < KernelStackBase ) ) {

        return STATUS_UNSUCCESSFUL;
    }

    Cached = KdpGetModuleByAddress( TargetContext->Rip );

    if ( Cached == NULL ) {

        TargetContext->Rip = KdpReadULong64( TargetContext->Rsp );
        TargetContext->Rsp += 8;
        return STATUS_SUCCESS;
    }

    if ( KiFastSystemCall == 0 ) {

        DbgGetFunctionByName( KdpKernelContext, L"KiFastSystemCall", ( ULONG* )&KiFastSystemCall );
        KiFastSystemCall += KdpKernelContext->Start;
        DbgFieldOffset( KdpKernelContext, L"_KTHREAD", L"SystemCall", &SystemCallOffset );
    }

    //
    // TODO: take exception directory as an argument and read once when unwinding
    // a complete frame, only re-read if the context changes, cache this too.
    //
    if ( !KdpReadDataDirectory( Cached->Start, IMAGE_DIRECTORY_ENTRY_EXCEPTION, NULL, &FunctionCount ) ) {

        TargetContext->Rip = KdpReadULong64( TargetContext->Rsp );
        TargetContext->Rsp += 8;
        return STATUS_SUCCESS;
    }

    FunctionTable = ( PIMAGE_RUNTIME_FUNCTION )OslAllocate( FunctionCount );
    FunctionCount /= sizeof( IMAGE_RUNTIME_FUNCTION );

    KdpReadDataDirectory( Cached->Start, IMAGE_DIRECTORY_ENTRY_EXCEPTION, FunctionTable, NULL );

    for ( CurrentFunction = 0; CurrentFunction < FunctionCount; CurrentFunction++ ) {

        if ( TargetContext->Rip >= Cached->Start + FunctionTable[ CurrentFunction ].BeginAddress &&
             TargetContext->Rip < Cached->Start + FunctionTable[ CurrentFunction ].EndAddress &&
             Cached->Start + FunctionTable[ CurrentFunction ].BeginAddress == KiFastSystemCall ) {

            KdpReadDebuggee( ( ULONG64 )Thread + SystemCallOffset,
                             sizeof( syscall ),
                             &syscall );
            TargetContext->Rip = syscall.PreviousIp;
            TargetContext->Rsp = syscall.PreviousStack;
            return STATUS_SUCCESS;
        }

        if ( TargetContext->Rip >= Cached->Start + FunctionTable[ CurrentFunction ].BeginAddress &&
             TargetContext->Rip < Cached->Start + FunctionTable[ CurrentFunction ].EndAddress ) {

            ntStatus = KdpUnwindPrologue( Thread,
                                          TargetContext,
                                          ( PVOID )Cached->Start,
                                          &FunctionTable[ CurrentFunction ] );
            OslFree( FunctionTable );
            return ntStatus;
        }
    }

    OslFree( FunctionTable );
    TargetContext->Rip = KdpReadULong64( TargetContext->Rsp );
    TargetContext->Rsp += 8;
    return STATUS_SUCCESS;
}

NTSTATUS
KdpUnwindPrologue(
    _In_ PKTHREAD                Thread,
    _In_ PCONTEXT                TargetContext,
    _In_ PVOID                   TargetBase,
    _In_ PIMAGE_RUNTIME_FUNCTION TargetFunction
)
{
    NTSTATUS ntStatus;
    PULONG64 GeneralRegisters;
    PUNWIND_INFO UnwindInfo;
    ULONG64 CurrentCode;
    UCHAR OpInfo;
    UCHAR UnwindOp;
    ULONG32 FrameOffset;
    ULONG64 StackBase;

    KdpGetThreadStack( Thread, &StackBase, NULL );

    //
    // GeneralRegisters should be an array of
    // registers ordered rax -> r15
    //

    GeneralRegisters = &TargetContext->Rax;
    UnwindInfo = ( PUNWIND_INFO )( ( PUCHAR )TargetBase + TargetFunction->UnwindData );//read in 

    UNWIND_INFO TempInfo;

    KdpReadDebuggee( ( ULONG64 )TargetBase + TargetFunction->UnwindData, sizeof( UNWIND_INFO ), &TempInfo );

    UnwindInfo = ( PUNWIND_INFO )OslAllocate( sizeof( UNWIND_INFO ) + TempInfo.CountOfCodes );

    KdpReadDebuggee( ( ULONG64 )TargetBase + TargetFunction->UnwindData,
                     sizeof( UNWIND_INFO ) + TempInfo.CountOfCodes,
                     UnwindInfo );

    for ( CurrentCode = 0; CurrentCode < UnwindInfo->CountOfCodes; ) {
        OpInfo = UnwindInfo->UnwindCode[ CurrentCode ].OpInfo;
        UnwindOp = UnwindInfo->UnwindCode[ CurrentCode ].UnwindOp;

        if ( ( ULONG64 )UnwindInfo->UnwindCode[ CurrentCode ].CodeOffset >
            ( TargetContext->Rip - ( ULONG64 )TargetBase + TargetFunction->BeginAddress ) ) {
            CurrentCode += KdpUnwindOpSlotTable[ UnwindOp ];

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
                TargetContext->Rsp += ( ULONG64 )OpInfo * 8 + 8;
                break;

            case UWOP_PUSH_NONVOL:;
                GeneralRegisters[ OpInfo ] = KdpReadULong64( TargetContext->Rsp );//*( PULONG64 )TargetContext->Rsp;
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
                GeneralRegisters[ OpInfo ] = KdpReadULong64( StackBase + FrameOffset );//*( PULONG64 )( StackBase + FrameOffset );
                break;

            case UWOP_SAVE_NONVOL_FAR:;
                CurrentCode += 2;
                FrameOffset = UnwindInfo->UnwindCode[ CurrentCode - 1 ].FrameOffset;
                FrameOffset += ( ULONG32 )UnwindInfo->UnwindCode[ CurrentCode ].FrameOffset << 16;

                GeneralRegisters[ OpInfo ] = KdpReadULong64( StackBase + FrameOffset );//*( PULONG64 )( StackBase + FrameOffset );
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

                    TargetContext->Rip = KdpReadULong64( TargetContext->Rsp + 8 );
                    TargetContext->Rsp = KdpReadULong64( TargetContext->Rsp + 32 );
                }
                else {

                    TargetContext->Rip = KdpReadULong64( TargetContext->Rsp );
                    TargetContext->Rsp = KdpReadULong64( TargetContext->Rsp + 24 );
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

        ntStatus = KdpUnwindPrologue( Thread,
                                      TargetContext,
                                      TargetBase,
                                      ( PIMAGE_RUNTIME_FUNCTION )(
                                      ( ULONG64 )TargetBase +
                                          FIELD_OFFSET( UNWIND_INFO, UnwindCode[ UnwindInfo->CountOfCodes + ( UnwindInfo->CountOfCodes & 1 ) ] ) ) );
        //( PIMAGE_RUNTIME_FUNCTION )( &UnwindInfo->UnwindCode[ UnwindInfo->CountOfCodes + ( UnwindInfo->CountOfCodes & 1 ) ] ) );
        OslFree( UnwindInfo );
        return ntStatus;
    }
    else {

        OslFree( UnwindInfo );
        TargetContext->Rip = KdpReadULong64( TargetContext->Rsp );//*( PULONG64 )( TargetContext->Rsp );
        TargetContext->Rsp += 8;
        return STATUS_SUCCESS;
    }
}
