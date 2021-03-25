


#include <carbsup.h>
#include "../hal/halp.h"
#include "../ke/ki.h"
#include "../io/iop.h"
#include "../rtl/rtlp.h"

BOOLEAN
RtlpTrapAssertionFailure(
    _In_ PKINTERRUPT Interrupt
)
{
    //
    // This routine is not finished, in the future, check the previousmode and 
    // probe the arguments.
    //
    // The structure for an int2c call is just rcx=string or rcx=exception code
    //

    RtlDebugPrint( L"Assertion Failure: %s %ull\n", ( PVOID* )Interrupt->TrapFrame->Rcx,
                   Interrupt->TrapFrame->Rip );

    if ( PsGetPreviousMode( PsGetCurrentThread( ) ) == KernelMode ) {

        KeBugCheck( STATUS_ASSERTION_FAILURE );
    }

    return TRUE;
}

VOID
RtlAssertionFailure(
    _In_ PWSTR Assert
)
{
    Assert;
    //RtlDebugPrint( L"Assert fail: %s %ull %ull\n", Assert, _ReturnAddress( ), _AddressOfReturnAddress( ) );
    __debugbreak( );
    //__int2c( );
}

NORETURN
VOID
RtlRaiseException(
    _In_ NTSTATUS Exception
)
{
    //
    // This is a support routine. (call KeBugCheckEx, usually)
    //

    //*( int* )0 = 1;

    Exception;
    if ( PsGetPreviousMode( PsGetCurrentThread( ) ) == KernelMode ) {

        KeBugCheck( STATUS_UNHANDLED_SYSTEM_EXCEPTION );
    }
    else {

        // terminate process.
        KeBugCheck( STATUS_UNHANDLED_SYSTEM_EXCEPTION );
    }

}

VOID
__C_specific_handler(
    _In_ PEXCEPTION_RECORD Record,
    _In_ PUNWIND_INFO      Unwind
)
{

    //
    // this function needs to be implemented such that
    // the exception dispatcher hands the thread control 
    // to this, and this calls ntcontinue or exits.
    // 
    // because of exception filtering, there is a chance an exception handler
    // exists, but is not called, or if a function contains a handler and there is a ball ache.
    //

    PULONG64 GeneralRegisters;
    PSCOPE_TABLE Scope;
    ULONG64 CurrentScope;
    BOOLEAN Filter;

    RtlDebugPrint( L"__c_specific_handler called!\n" );

    GeneralRegisters = &Record->ExceptionContext.Rax;
    //Scope = ( PVOID )( Record->Vad->Start + *( ULONG32* )( ( ULONG64 )Unwind + Unwind->CountOfCodes * sizeof( UNWIND_CODE ) ) + 8 );
    Scope = ( PVOID )( ( ULONG64 )Unwind + 8 +
                       sizeof( UNWIND_CODE ) * Unwind->CountOfCodes +
                       ( sizeof( UNWIND_CODE ) * Unwind->CountOfCodes ) % 4 );

    for ( CurrentScope = 0; CurrentScope < Scope->Count; CurrentScope++ ) {

        if ( Record->ExceptionContext.Rip >= Record->Vad->Start + Scope->ScopeRecord[ CurrentScope ].BeginAddress &&
             Record->ExceptionContext.Rip < Record->Vad->Start + Scope->ScopeRecord[ CurrentScope ].EndAddress ) {

            if ( Scope->ScopeRecord[ CurrentScope ].HandlerAddress != 1 ) {

                Filter = TRUE;
                //Filter = ( ( BOOLEAN( *)( ) )( Record->Vad->Start + Scope->ScopeRecord[ CurrentScope ].HandlerAddress ) )( );
            }
            else {

                Filter = TRUE;
            }

            if ( !Filter ) {

                break;
            }

            if ( Unwind->FrameRegister != 0 ) {

                GeneralRegisters[ Unwind->FrameRegister ] = Record->ExceptionContext.Rsp + 16 * Unwind->FrameOffset;
            }

            Record->ExceptionContext.Rip = Record->Vad->Start + Scope->ScopeRecord[ CurrentScope ].JumpTarget;

            RtlDebugPrint( L"brual rip=%ull rsp=%ull irql=%d if=%d\n",
                           Record->ExceptionContext.Rip,
                           Record->ExceptionContext.Rsp,
                           KeGetCurrentIrql( ),
                           __readeflags( ) );
            ZwContinue( &Record->ExceptionContext );
        }
    }


    __int2c( );
}

BOOLEAN
RtlpHandleHighIrqlException(
    _In_ PEXCEPTION_RECORD Record,
    _In_ PUNWIND_INFO      Unwind,
    _In_ PKTRAP_FRAME      TrapFrame
)
{

    //
    // this function needs to be implemented such that
    // the exception dispatcher hands the thread control 
    // to this, and this calls ntcontinue or exits.
    // 
    // because of exception filtering, there is a chance an exception handler
    // exists, but is not called, or if a function contains a handler and there is a ball ache.
    //

    PULONG64 GeneralRegisters;
    PSCOPE_TABLE Scope;
    ULONG64 CurrentScope;
    BOOLEAN Filter;

    RtlDebugPrint( L"RtlpHandleHighIrqlException called!\n" );

    GeneralRegisters = &Record->ExceptionContext.Rax;
    //Scope = ( PVOID )( Record->Vad->Start + *( ULONG32* )( ( ULONG64 )Unwind + Unwind->CountOfCodes * sizeof( UNWIND_CODE ) ) + 8 );
    Scope = ( PVOID )( ( ULONG64 )Unwind + 8 +
                       sizeof( UNWIND_CODE ) * Unwind->CountOfCodes +
                       ( sizeof( UNWIND_CODE ) * Unwind->CountOfCodes ) % 4 );

    for ( CurrentScope = 0; CurrentScope < Scope->Count; CurrentScope++ ) {

        if ( Record->ExceptionContext.Rip >= Record->Vad->Start + Scope->ScopeRecord[ CurrentScope ].BeginAddress &&
             Record->ExceptionContext.Rip < Record->Vad->Start + Scope->ScopeRecord[ CurrentScope ].EndAddress ) {

            if ( Scope->ScopeRecord[ CurrentScope ].HandlerAddress != 1 ) {

                Filter = TRUE;
                //Filter = ( ( BOOLEAN( *)( ) )( Record->Vad->Start + Scope->ScopeRecord[ CurrentScope ].HandlerAddress ) )( );
            }
            else {

                Filter = TRUE;
            }

            if ( !Filter ) {

                break;
            }

            Record->ExceptionContext.Rip = Record->Vad->Start + Scope->ScopeRecord[ CurrentScope ].JumpTarget;

            RtlContextToTrapFrame( TrapFrame, &Record->ExceptionContext );
            RtlDebugPrint( L"pog. rip=%ull rsp=%ull\n",
                           TrapFrame->Rip,
                           TrapFrame->Rsp );
            return TRUE;
        }
    }

    return FALSE;
}
