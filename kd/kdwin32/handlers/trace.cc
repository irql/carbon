


#include "../carbon.h"
#include "../kd.h"
#include "../kdp.h"
#include "../pdb.h"
#include "../osl/osl.h"

typedef UCHAR KX86_REG[ 10 ];
typedef UCHAR KXMM_REG[ 16 ];

typedef union _KMMX_REG {
    struct {
        KX86_REG St;
        UCHAR    StReserved[ 6 ];
    };
    struct {
        UCHAR    MmValue[ 8 ];
        UCHAR    MmReserved[ 8 ];
    };
} KMMX_REG, *PKMMX_REG;

typedef struct _KFXSAVE64 {
    USHORT      Control;
    USHORT      Status;
    UCHAR       Tag;
    UCHAR       Reserved1;
    USHORT      Opcode;
    ULONG64     Ip64;
    ULONG64     Dp64;
    ULONG32     Mxcsr;
    ULONG32     MxcsrMask;
    KMMX_REG    MmxReg[ 8 ];
    KXMM_REG    XmmReg[ 16 ];
    UCHAR       Reserved2[ 48 ];
    UCHAR       Available[ 48 ];
} KFXSAVE64, *PKFXSAVE64;

typedef struct _KTRAP_FRAME {
    KFXSAVE64 Fxsave64;
    ULONG64   Align2;
    ULONG64   Align1;
    ULONG64   Align0;
    ULONG64   Dr7;
    ULONG64   Dr6;
    ULONG64   Dr3;
    ULONG64   Dr2;
    ULONG64   Dr1;
    ULONG64   Dr0;
    ULONG64   Cr3;
    ULONG64   SegGs;
    ULONG64   SegFs;
    ULONG64   SegEs;
    ULONG64   SegDs;
    ULONG64   R15;
    ULONG64   R14;
    ULONG64   R13;
    ULONG64   R12;
    ULONG64   R11;
    ULONG64   R10;
    ULONG64   R9;
    ULONG64   R8;
    ULONG64   Rdi;
    ULONG64   Rsi;
    ULONG64   Rbp;
    ULONG64   Rbx;
    ULONG64   Rdx;
    ULONG64   Rcx;
    ULONG64   Rax;
    ULONG64   Interrupt;
    ULONG64   Error;
    ULONG64   Rip;
    ULONG64   SegCs;
    ULONG64   EFlags;
    ULONG64   Rsp;
    ULONG64   SegSs;

} KTRAP_FRAME, *PKTRAP_FRAME;

#define RtlTrapFrameToContext( TrapFrame, Context ) \
( Context )->SegCs = ( TrapFrame )->SegCs; \
( Context )->SegSs = ( TrapFrame )->SegSs; \
( Context )->SegDs = ( TrapFrame )->SegDs; \
( Context )->SegEs = ( TrapFrame )->SegEs; \
( Context )->SegFs = ( TrapFrame )->SegFs; \
( Context )->SegGs = ( TrapFrame )->SegGs; \
( Context )->EFlags = ( TrapFrame )->EFlags; \
( Context )->Rax = ( TrapFrame )->Rax; \
( Context )->Rcx = ( TrapFrame )->Rcx; \
( Context )->Rdx = ( TrapFrame )->Rdx; \
( Context )->Rbx = ( TrapFrame )->Rbx; \
( Context )->Rsp = ( TrapFrame )->Rsp; \
( Context )->Rbp = ( TrapFrame )->Rbp; \
( Context )->Rsi = ( TrapFrame )->Rsi; \
( Context )->Rdi = ( TrapFrame )->Rdi; \
( Context )->R8 = ( TrapFrame )->R8; \
( Context )->R9 = ( TrapFrame )->R9; \
( Context )->R10 = ( TrapFrame )->R10; \
( Context )->R11 = ( TrapFrame )->R11; \
( Context )->R12 = ( TrapFrame )->R12; \
( Context )->R13 = ( TrapFrame )->R13; \
( Context )->R14 = ( TrapFrame )->R14; \
( Context )->R15 = ( TrapFrame )->R15; \
( Context )->Rip = ( TrapFrame )->Rip; \
( Context )->Dr0 = ( TrapFrame )->Dr0; \
( Context )->Dr1 = ( TrapFrame )->Dr1; \
( Context )->Dr2 = ( TrapFrame )->Dr2; \
( Context )->Dr3 = ( TrapFrame )->Dr3; \
( Context )->Dr6 = ( TrapFrame )->Dr6; \
( Context )->Dr7 = ( TrapFrame )->Dr7


#define RtlContextToTrapFrame( TrapFrame, Context ) \
( TrapFrame )->SegCs = ( Context )->SegCs; \
( TrapFrame )->SegSs = ( Context )->SegSs; \
( TrapFrame )->SegDs = ( Context )->SegDs; \
( TrapFrame )->SegEs = ( Context )->SegEs; \
( TrapFrame )->SegFs = ( Context )->SegFs; \
( TrapFrame )->SegGs = ( Context )->SegGs; \
( TrapFrame )->EFlags = ( Context )->EFlags; \
( TrapFrame )->Rax = ( Context )->Rax; \
( TrapFrame )->Rcx = ( Context )->Rcx; \
( TrapFrame )->Rdx = ( Context )->Rdx; \
( TrapFrame )->Rbx = ( Context )->Rbx; \
( TrapFrame )->Rsp = ( Context )->Rsp; \
( TrapFrame )->Rbp = ( Context )->Rbp; \
( TrapFrame )->Rsi = ( Context )->Rsi; \
( TrapFrame )->Rdi = ( Context )->Rdi; \
( TrapFrame )->R8 = ( Context )->R8; \
( TrapFrame )->R9 = ( Context )->R9; \
( TrapFrame )->R10 = ( Context )->R10; \
( TrapFrame )->R11 = ( Context )->R11; \
( TrapFrame )->R12 = ( Context )->R12; \
( TrapFrame )->R13 = ( Context )->R13; \
( TrapFrame )->R14 = ( Context )->R14; \
( TrapFrame )->R15 = ( Context )->R15; \
( TrapFrame )->Rip = ( Context )->Rip; \
( TrapFrame )->Dr0 = ( Context )->Dr0; \
( TrapFrame )->Dr1 = ( Context )->Dr1; \
( TrapFrame )->Dr2 = ( Context )->Dr2; \
( TrapFrame )->Dr3 = ( Context )->Dr3; \
( TrapFrame )->Dr6 = ( Context )->Dr6; \
( TrapFrame )->Dr7 = ( Context )->Dr7

VOID
KdpHandleTraceStack(
    _In_ PKD_COMMAND Command
)
{

    HRESULT hResult;
    NTSTATUS ntStatus;
    PWCHAR FileName;
    PWCHAR FunctionName;
    LONG   FunctionDisplacement;
    ULONG  LineNumber;

    PKD_CACHED_MODULE Cached;

    static LONG TrapFrameOffset = 0;

    KTRAP_FRAME TrapFrame;
    CONTEXT Context;

    switch ( Command->Arguments[ 0 ].String[ 1 ] ) {
    case 't': // trace stack of thread

        if ( Command->ArgumentCount != 3 ||
             Command->Arguments[ 1 ].ArgumentType != KdArgumentInteger ||
             Command->Arguments[ 2 ].ArgumentType != KdArgumentEol ) {

            OslWriteConsole( L"syntax error.\n" );
            break;
        }

        if ( TrapFrameOffset == 0 ) {

            DbgFieldOffset( KdpKernelContext, L"_KTHREAD", L"TrapFrame", &TrapFrameOffset );
        }

        KdpReadDebuggee( ( ULONG64 )Command->Arguments[ 1 ].Integer + TrapFrameOffset,
                         sizeof( KTRAP_FRAME ),
                         &TrapFrame );

        RtlTrapFrameToContext( &TrapFrame, &Context );

        do {

            Cached = KdpGetModuleByAddress( Context.Rip );

            if ( Cached == NULL ) {

                OslWriteConsole( L"              0x%p\n",
                                 Context.Rip );
            }
            else {

                hResult = DbgGetFunctionByAddress( Cached,
                                                   Context.Rip - Cached->Start,
                                                   &FunctionName,
                                                   &FileName,
                                                   &LineNumber,
                                                   &FunctionDisplacement );

                if ( hResult != 0 ) {

                    OslWriteConsole( L"  %30s 0x%p (0x%08x)\n",
                                     Cached->Name,
                                     Context.Rip,
                                     Context.Rip - Cached->Start );
                }
                else {
                    OslWriteConsole( L"  %30s %48s:%04d %s+%d\n", Cached->Name,
                                     FileName, LineNumber, FunctionName, FunctionDisplacement );
                }
            }

            ntStatus = KdpUnwindFrame( ( PKTHREAD )Command->Arguments[ 1 ].Integer,
                                       &Context );
        } while ( NT_SUCCESS( ntStatus ) );

        OslWriteConsole( L"\n" );

        break;
    default:
        OslWriteConsole( L"unrecognised format.\n" );
        break;
    }
}
