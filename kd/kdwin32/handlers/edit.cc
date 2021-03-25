


#include <windows.h>
#include <dia2.h>
#include "../kd.h"
#include "../pdb.h"
#include "../osl/osl.h"

VOID
KdpHandleEdit(
    _In_ PKD_COMMAND Command
)
{
    PKD_CACHED_MODULE Module;
    VOID( *KdpWrite )( ULONG64, ULONG64 );
    ULONG Rva;

    KdpWrite = NULL;

    switch ( Command->Arguments[ 0 ].String[ 1 ] ) {
    case 'q': // ulong64
        KdpWrite = ( decltype( KdpWrite ) )KdpWriteULong64;
    case 'd': // ulong32
        if ( KdpWrite == NULL ) {

            KdpWrite = ( decltype( KdpWrite ) )KdpWriteULong32;
        }
    case 's': // short
    case 'w': // word
        if ( KdpWrite == NULL ) {

            KdpWrite = ( decltype( KdpWrite ) )KdpWriteUShort;
        }
    case 'c': // char
    case 'b': // byte
        if ( KdpWrite == NULL ) {

            KdpWrite = ( decltype( KdpWrite ) )KdpWriteUChar;
        }

        if ( Command->ArgumentCount == 4 &&
             Command->Arguments[ 1 ].ArgumentType == KdArgumentInteger &&
             Command->Arguments[ 2 ].ArgumentType == KdArgumentInteger &&
             Command->Arguments[ 3 ].ArgumentType == KdArgumentEol ) {

            KdpWrite( Command->Arguments[ 1 ].Integer, Command->Arguments[ 2 ].Integer );
        }
        else if ( Command->ArgumentCount == 6 &&
                  Command->Arguments[ 1 ].ArgumentType == KdArgumentString &&
                  Command->Arguments[ 2 ].ArgumentType == KdArgumentExclamationPoint &&
                  Command->Arguments[ 3 ].ArgumentType == KdArgumentString &&
                  Command->Arguments[ 4 ].ArgumentType == KdArgumentInteger &&
                  Command->Arguments[ 5 ].ArgumentType == KdArgumentEol ) {

            Module = KdpGetModuleByShort( Command->Arguments[ 1 ].String );

            if ( Module != NULL ) {

                if ( DbgGetAddressByName( Module, Command->Arguments[ 3 ].String, &Rva ) == S_OK ) {

                    KdpWrite( Module->Start + Rva, Command->Arguments[ 4 ].Integer );
                }
                else {

                    OslWriteConsole( L"failed.\n" );
                }
            }
        }
        else {

            OslWriteConsole( L"syntax error.\n" );
        }

        break;
    default:
        OslWriteConsole( L"unrecognised format.\n" );
        break;
    }

}
