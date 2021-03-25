


#include "../carbon.h"
#include "../kd.h"
#include "../kdp.h"
#include "../pdb.h"
#include "../osl/osl.h"

VOID
KdpHandleList(
    _In_ PKD_COMMAND Command
)
{
    STATIC LONG VadRootOffset = 0;
    STATIC LONG ProcessThreadLinksOffset = 0;
    STATIC LONG ThreadLinksOffset = 0;
    STATIC LONG ThreadTrapFrameOffset = 0;
    STATIC LONG TrapFrameRipOffset = 0;
    STATIC LONG ThreadIdOffset = 0;
    STATIC LONG ProcessLinksOffset = 0;
    STATIC LONG ProcessIdOffset = 0;
    ULONG64 VadAddress;
    ULONG64 ThreadLinkAddress;
    ULONG64 InitialThreadLink;
    ULONG64 ThreadAddress;
    WCHAR NameBuffer[ 256 ];
    WCHAR ShortNameBuffer[ 256 ];
    MM_VAD Vad;
    HRESULT hResult;
    PWCHAR FileName;
    PWCHAR FunctionName;
    LONG   FunctionDisplacement;
    ULONG  LineNumber;
    PKD_CACHED_MODULE Cached;
    ULONG64 Rip;
    ULONG64 ThreadId;
    ULONG64 ProcessLinkAddress;
    ULONG64 InitialProcessLink;
    ULONG64 ProcessAddress;

    if ( VadRootOffset == 0 ) {

        DbgFieldOffset( KdpKernelContext, L"_KPROCESS", L"VadRoot", &VadRootOffset );
        DbgFieldOffset( KdpKernelContext, L"_KPROCESS", L"ThreadLinks", &ProcessThreadLinksOffset );
        DbgFieldOffset( KdpKernelContext, L"_KTHREAD", L"ThreadLinks", &ThreadLinksOffset );
        DbgFieldOffset( KdpKernelContext, L"_KTHREAD", L"TrapFrame", &ThreadTrapFrameOffset );
        DbgFieldOffset( KdpKernelContext, L"_KTRAP_FRAME", L"Rip", &TrapFrameRipOffset );
        DbgFieldOffset( KdpKernelContext, L"_KTHREAD", L"ThreadId", &ThreadIdOffset );
        DbgFieldOffset( KdpKernelContext, L"_KPROCESS", L"ProcessLinks", &ProcessLinksOffset );
        DbgFieldOffset( KdpKernelContext, L"_KPROCESS", L"ProcessId", &ProcessIdOffset );
    }

    switch ( Command->Arguments[ 0 ].String[ 1 ] ) {
    case 'm'://module
        // WARNING: PROCESS VAD SPECIFIC ONLY!

        if ( Command->Arguments[ 1 ].ArgumentType != KdArgumentEol ) {

            OslWriteConsole( L"syntax error.\n" );
            break;
        }

        OslWriteConsole( L"        NAME"
                         L"              START"
                         L"                END"
                         L"             CHARGE\n" );

        VadAddress = KdpReadULong64( ( ULONG64 )KdpProcess + VadRootOffset );
        do {

            KdpReadDebuggee( VadAddress, sizeof( MM_VAD ), &Vad );

            KdpGetVadFileName( &Vad, NameBuffer, NULL );
            KdpGetShortName( NameBuffer, ShortNameBuffer );

            OslWriteConsole( L"%12s 0x%p 0x%p 0x%p\n",
                             ShortNameBuffer,
                             Vad.Start,
                             Vad.End,
                             Vad.Charge );

            VadAddress = ( ULONG64 )Vad.Link;
        } while ( Vad.Link != NULL );

        break;
    case 't'://thread
        // WARNING, again process specific.

        if ( Command->Arguments[ 1 ].ArgumentType != KdArgumentEol ) {

            OslWriteConsole( L"syntax error.\n" );
            break;
        }

        OslWriteConsole( L"      THREAD ADDRESS (  ID)\n"
                         L"                          MODULE"
                         L"                                      SOURCE FILE:LINE Function+Offset\n" );

        ThreadLinkAddress = KdpReadULong64( ( ULONG64 )KdpProcess + ProcessThreadLinksOffset );
        InitialThreadLink = ThreadLinkAddress;
        do {
            ThreadAddress = ThreadLinkAddress - ThreadLinksOffset;
            Rip = KdpReadULong64( ThreadAddress +
                                  ThreadTrapFrameOffset +
                                  TrapFrameRipOffset );
            ThreadId = KdpReadULong64( ThreadAddress + ThreadIdOffset );

            OslWriteConsole( L"  0x%p (%04d)\n",
                             ThreadAddress,
                             ThreadId );

            Cached = KdpGetModuleByAddress( Rip );

            if ( Cached == NULL ) {

                OslWriteConsole( L"  0x%p\n", Rip );
                break;
            }

            hResult = DbgGetFunctionByAddress( Cached,
                                               Rip - Cached->Start,
                                               &FunctionName,
                                               &FileName,
                                               &LineNumber,
                                               &FunctionDisplacement );

            if ( hResult != 0 ) {

                OslWriteConsole( L"  %30s 0x%p (0x%08x)\n",
                                 Cached->Name,
                                 Rip,
                                 Rip - Cached->Start );
                break;
            }

            OslWriteConsole( L"  %30s %48s:%04d %s+%d\n",
                             Cached->Name,
                             FileName,
                             LineNumber,
                             FunctionName,
                             FunctionDisplacement );

            ThreadLinkAddress = KdpReadULong64( ThreadLinkAddress );
        } while ( ThreadLinkAddress != InitialThreadLink );

        break;
    case 'p'://process

        if ( Command->Arguments[ 1 ].ArgumentType != KdArgumentEol ) {

            OslWriteConsole( L"syntax error.\n" );
            break;
        }

        ProcessLinkAddress = KdpReadULong64( ( ULONG64 )KdpSystemProcess + ProcessLinksOffset );
        InitialProcessLink = ProcessLinkAddress;
        do {
            ProcessAddress = ProcessLinkAddress - ProcessLinksOffset;

            VadAddress = KdpReadULong64( ProcessAddress + VadRootOffset );
            KdpReadDebuggee( VadAddress, sizeof( MM_VAD ), &Vad );
            KdpGetVadFileName( &Vad, NameBuffer, NULL );
            KdpGetShortName( NameBuffer, ShortNameBuffer );

            OslWriteConsole( L"  %12s 0x%p (%04d)\n",
                             ShortNameBuffer,
                             ProcessAddress,
                             KdpReadULong64( ProcessAddress + ProcessIdOffset ) );

            ProcessLinkAddress = KdpReadULong64( ProcessLinkAddress );
        } while ( ProcessLinkAddress != InitialProcessLink );

        break;
    default:

        OslWriteConsole( L"unrecognised format.\n" );
        break;
    }

}
