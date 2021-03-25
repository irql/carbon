


#include <carbsup.h>
#include "../hal/halp.h"
#include "ki.h"
#include "../rtl/rtlp.h"
#include "../mm/mi.h"
#include "../rtl/ldr/ldrp.h"
#include "../io/iop.h"
#include "../ps/psp.h"

VOLATILE ULONG64 KiProcessorControl = 0;
VOLATILE ULONG64 KiProcessorSkip = 0;
BOOLEAN          KdDebuggerEnabled = 0;

VOID
KiTrapProcessorControl(

)
{
    if ( KeQueryCurrentProcessor( )->ProcessorNumber == KiProcessorSkip ) {

        return;
    }

    HalLocalApicWrite( LAPIC_LVT_TIMER_REGISTER,
                       HalLocalApicRead( LAPIC_LVT_TIMER_REGISTER ) |
                       LAPIC_MASKED );

    _InterlockedIncrement64( ( LONG64* )&KiProcessorControl );

    while ( 1 ) {

        __halt( );
    }
}

VOID
KiProcessorShutdown(

)
{

    HalLocalApicWrite( LAPIC_LVT_TIMER_REGISTER,
                       HalLocalApicRead( LAPIC_LVT_TIMER_REGISTER ) |
                       LAPIC_MASKED );

    KiProcessorControl = 0;
    KiProcessorSkip = KeQueryCurrentProcessor( )->ProcessorNumber;

    KeGenericCallIpi( ( PKIPI_CALL )KiTrapProcessorControl,
                      NULL );

    while ( KiProcessorControl != ( KeQueryProcessorCount( ) - 1 ) )
        ;//_mm_pause( );
}

NORETURN
VOID
KiBugCheckFromRecord(
    _In_ PEXCEPTION_RECORD Record
)
{
    KIRQL PreviousIrql;

    //
    //

    if ( KdDebuggerEnabled ) {

        VOID( *_KdDispatchException )(
            _In_ PEXCEPTION_RECORD ExceptionRecord
            );
        UNICODE_STRING Kd = RTL_CONSTANT_STRING( L"KDCOM.SYS" );

        LdrGetExportAddressByName( ( PVOID )MiFindVadByShortName( PsInitialSystemProcess, &Kd )->Start,
                                   "KdDispatchException", ( PVOID* )&_KdDispatchException );

        _KdDispatchException( Record );
    }

    //
    //

    KeRaiseIrql( DISPATCH_LEVEL, &PreviousIrql );

    KeQueryCurrentProcessor( )->InService = TRUE;
    KeQueryCurrentProcessor( )->PreviousService = 0;

    RtlDebugPrint( L"KiBugCheckFromRecord:\n"
                   L"thread id=%d\n"
                   L"processor mode=%s\n"
                   L"processor number=%d\n"
                   L"status code=%d\n"
                   L"rip=%#.16ull\n"
                   L"rsp=%#.16ull\n",
                   PsGetThreadId( Record->Thread ),
                   Record->ProcessorMode == UserMode ? L"UserMode" : L"KernelMode",
                   Record->Thread->ProcessorNumber,
                   Record->Status,
                   Record->ExceptionContext.Rip,
                   Record->ExceptionContext.Rsp );

#if 0

    PMM_VAD Vad;

    do {
        if ( Record.ProcessorMode == KernelMode ) {

            Vad = MiFindVadByAddress( PsInitialSystemProcess,
                                      OriginalContext.Rip );
        }
        else {

            Vad = MiFindVadByAddress( PsGetThreadProcess( Record.Thread ),
                                      OriginalContext.Rip );
        }

        if ( Vad == NULL ) {

            ntStatus = RtlUnwindFrame( Record.Thread,
                                       &OriginalContext,
                                       Record.Thread->StackBase,
                                       Record.Thread->StackLength );
        }
        else {

            RtlDebugPrint( L"rip=%s!%ull rsp=%ull\n",
                           &Vad->FileObject->FileName.Buffer[
                               FsRtlFileNameIndex( &Vad->FileObject->FileName ) ],
                           OriginalContext.Rip - Vad->Start,
                                   OriginalContext.Rsp );

            ntStatus = RtlUnwindFrame( Record.Thread,
                                       &OriginalContext,
                                       Record.Thread->StackBase,
                                       Record.Thread->StackLength );
        }

    } while ( NT_SUCCESS( ntStatus ) );

#endif

    KiProcessorShutdown( ); // shutdown after kd invoked.

    while ( 1 ) {

        __halt( );
    }
}

NORETURN
VOID
KeBugCheckEx(
    _In_ NTSTATUS Status,
    _In_ ULONG64  Code1,
    _In_ ULONG64  Code2,
    _In_ ULONG64  Code3,
    _In_ ULONG64  Code4
)
{
    EXCEPTION_RECORD Record = { 0 };

    Record.Status = Status;
    Record.Code1 = Code1;
    Record.Code2 = Code2;
    Record.Code3 = Code3;
    Record.Code4 = Code4;

    Record.ExceptionContext.Rip = ( ULONG64 )_ReturnAddress( );
    Record.ExceptionContext.Rsp = ( ULONG64 )_AddressOfReturnAddress( ) + 8;
    Record.ExceptionSeverity = ExceptionFatal;
    Record.ProcessorMode = KernelMode;
    Record.Thread = PsGetCurrentThread( );
    Record.Vad = MiFindVadByAddress( PsGetThreadProcess( Record.Thread ),
                                     Record.ExceptionContext.Rip );

    KiBugCheckFromRecord( &Record );
}

NORETURN
VOID
KeBugCheck(
    _In_ NTSTATUS Status
)
{
    EXCEPTION_RECORD Record = { 0 };

    Record.Status = Status;

    Record.ExceptionContext.Rip = ( ULONG64 )_ReturnAddress( );
    Record.ExceptionContext.Rsp = ( ULONG64 )_AddressOfReturnAddress( ) + 8;
    Record.ExceptionSeverity = ExceptionFatal;
    Record.ProcessorMode = KernelMode;
    Record.Thread = PsGetCurrentThread( );
    Record.Vad = MiFindVadByAddress( PsGetThreadProcess( Record.Thread ),
                                     Record.ExceptionContext.Rip );

    KiBugCheckFromRecord( &Record );
}

VOID
KiExceptionDispatch(
    _In_ PKTRAP_FRAME TrapFrame
)
{
    NTSTATUS ntStatus;

    PEXCEPTION_RECORD Record;

    CONTEXT OriginalContext;
    PUNWIND_INFO Unwind;
    PEXCEPTION_HANDLER Handler;

    PPS_SYSTEM_STACK Stack;
    KPROCESSOR_MODE ExceptMode;
    ULONG64 HandleStack;
    BOOLEAN HandleCapable;
    BOOLEAN HandleComplete;

    HandleComplete = FALSE;
    HandleCapable = TRUE;

    ExceptMode = ( TrapFrame->SegCs & 1 ) == 1 ? UserMode : KernelMode;

    if ( ExceptMode == KernelMode ) {

        Stack = PspQueryStack( PsInitialSystemProcess, ( PVOID )TrapFrame->Rsp );
    }
    else {

        Stack = PspQueryStack( PsGetThreadProcess( PsGetCurrentThread( ) ), ( PVOID )TrapFrame->Rsp );
    }

    if ( Stack == NULL ) {

        //
        // This exception cannot be handled.. 
        // well it could if this was ExceptMode=KernelMode
        //

        Record = ( PEXCEPTION_RECORD )_alloca( sizeof( EXCEPTION_RECORD ) );
        HandleStack = NULL;
        HandleCapable = FALSE;
    }
    else if ( ExceptMode == KernelMode ) {

        //
        // TrapFrame.Rsp points to the current stack so...
        //

        Record = ( PEXCEPTION_RECORD )_alloca( sizeof( EXCEPTION_RECORD ) );
        HandleStack = NULL;
        HandleCapable = TRUE;

    }
    else {

        // range check.
        Record = ( PEXCEPTION_RECORD )( TrapFrame->Rsp - sizeof( EXCEPTION_RECORD ) );
        HandleStack = TrapFrame->Rsp - sizeof( EXCEPTION_RECORD ) - 0x30;
        HandleCapable = TRUE;
    }

    RtlDebugPrint( L"excpt: %d rip=%ull rsp=%ull\n",
                   TrapFrame->Interrupt, TrapFrame->Rip, TrapFrame->Rsp );

#if 0
    InterruptingService = KeQueryCurrentProcessor( )->InService;
    if ( InterruptingService ) {

        PreviousService = KeQueryCurrentProcessor( )->PreviousService;

        if ( PreviousService < 32 ) {

            //
            // Has there been an exception inside KiExceptionDispatch?
            //
            // This could raise a double fault.
            //

            KeRaiseIrql( DISPATCH_LEVEL, &PreviousIrql );

            RtlDebugPrint( L"KiExceptionDispatch double fault. %ull\n", TrapFrame->Rip );

            KiProcessorShutdown( );

            while ( 1 ) {

                __halt( );
            }
        }
    }
#endif

    //
    // Re-write a lot of these functions to process an exception_record instead
    // of taking in generic args
    //

    Record->Status = ( ULONG32 )TrapFrame->Interrupt;
    Record->Code1 = 0;
    Record->Code2 = 0;
    Record->Code3 = 0;
    Record->Code4 = 0;

    Record->Thread = PsGetCurrentThread( );
    Record->ProcessorMode = ExceptMode;
    Record->ExceptionSeverity = ExceptionNormal;

    RtlTrapFrameToContext( TrapFrame, &Record->ExceptionContext );
    RtlTrapFrameToContext( TrapFrame, &OriginalContext );

    Handler = NULL;
    Unwind = NULL;

    if ( HandleCapable ) {

        do {
            if ( Record->ProcessorMode == KernelMode ) {

                Stack = PspQueryStack( PsInitialSystemProcess,
                    ( PVOID )Record->ExceptionContext.Rsp );

                if ( Stack == NULL ) {

                    break;
                }

                Record->Vad = MiFindVadByAddress( PsInitialSystemProcess,
                                                  Record->ExceptionContext.Rip );
            }
            else {

                Stack = PspQueryStack( PsGetThreadProcess( Record->Thread ),
                    ( PVOID )Record->ExceptionContext.Rsp );

                if ( Stack == NULL ) {

                    break;
                }

                Record->Vad = MiFindVadByAddress( PsGetThreadProcess( Record->Thread ),
                                                  Record->ExceptionContext.Rip );
            }

            if ( Record->Vad != NULL ) {

                ntStatus = RtlpFindExceptionHandler( Record,
                                                     Record->Vad,
                                                     &Handler,
                                                     &Unwind );

                if ( ntStatus != STATUS_NOT_FOUND ) {

                    break;
                }
            }

            ntStatus = RtlUnwindFrame( Record->Thread,
                                       &Record->ExceptionContext,
                                       Stack->Address,
                                       Stack->Length );

        } while ( NT_SUCCESS( ntStatus ) );
    }

    if ( Handler != NULL ) {

        //
        // The exception has a handler and we can/should (change this to)
        // set the current context to this handler. The thread can then deal with 
        // the exception using NtContinue.
        //
        // Changes need to be made because an EHANDLER can exist, but could filter
        // 0, which means continue searching for handlers - we don't continue searching.
        //

        RtlDebugPrint( L"exception handled. rip=%ull\n",
                       TrapFrame->Rip );

        //if ( KeGetCurrentIrql( ) >= DISPATCH_LEVEL ) {
        if ( ExceptMode == KernelMode ) {

            //
            // If this exception occurred in a high irql task
            // then we need to take a slightly different route
            // because the thread dispatcher isn't running and
            // the default __c_specific_handler wont work.
            //
            // Can safely assume ExceptMode=KernelMode
            //

            //NT_ASSERT( ExceptMode == KernelMode );

            HandleComplete = RtlpHandleHighIrqlException( Record,
                                                          Unwind,
                                                          TrapFrame );
        }
        else {

            NT_ASSERT( KeGetCurrentIrql( ) == PASSIVE_LEVEL );

            TrapFrame->Rsp = HandleStack;
            TrapFrame->Rip = ( ULONG64 )Handler;
            TrapFrame->Rcx = ( ULONG64 )Record;
            TrapFrame->Rdx = ( ULONG64 )Unwind;

            RtlDebugPrint( L"exception handled. rip=%ull rsp=%ull\n",
                           TrapFrame->Rip,
                           TrapFrame->Rsp );

            HandleComplete = TRUE;
        }

        if ( HandleComplete ) {

            return;
        }
    }
    else {

        //
        // Restore the old context, we didn't find any exception handler
        //

        RtlCopyMemory( &Record->ExceptionContext, &OriginalContext, sizeof( CONTEXT ) );
    }

    //
    // Kd is invoked from KiBugCheckFromRecord
    //
    // No longer able to be recovered from this point onwards, irql
    // is raised and service status is set.
    //

    KiBugCheckFromRecord( Record );
}
