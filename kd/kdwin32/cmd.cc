


#include "carbon.h"
#include "kd.h"
#include "kdp.h"
#include "pdb.h"
#include "osl/osl.h"

PKD_HANDLE_COMMAND KdpHandleCommands[ ] = {
    KdpHandleContinue,
    KdpHandleDisplay,
    KdpHandleEdit,
    KdpHandleList,
    KdpHandleProcess,
    KdpHandleTraceStack,
    KdpHandleWsl
};

PCWSTR KdpCommandStrings[ ] = {
    L"g",       // Continue
    L"d?",      // Display
    L"e?",      // Edit
    L"l?",      // List
    L"set",     // set process
    L"k?",      // track stack
    L"wsl?",    // working set list
};

#define MAX_COMMANDS ( sizeof( KdpHandleCommands ) / sizeof( PVOID ) )

VOID
KdpSanitizeInput(
    _In_ PWSTR Buffer
)
{
    ULONG64 CurrentChar;

    for ( CurrentChar = 0; Buffer[ CurrentChar ] != 0; CurrentChar++ ) {

        if ( CurrentChar == 0 &&
             Buffer[ CurrentChar ] == ' ' ) {

            //
            // Remove any spaces at the beginning
            //

            RtlMoveMemory( Buffer, Buffer + 1, lstrlenW( ( PCWSTR )Buffer ) * sizeof( WCHAR ) );
            CurrentChar = 0;
            continue;
        }

        if ( Buffer[ CurrentChar ] == '\n' ||
             Buffer[ CurrentChar ] == '\r' ) {

            Buffer[ CurrentChar ] = 0;
        }
    }
}

PKD_COMMAND
KdpProcessCommand(
    _In_ PWSTR Buffer
)
{
    PKD_COMMAND Command;
    ULONG64 ArgumentCount;
    ULONG64 CurrentChar;
    ULONG64 CurrentChar1;

    ArgumentCount = 1; // Eol
    CurrentChar = 0;
    while ( 1 ) {

        if ( Buffer[ CurrentChar ] == ' ' ) {

            CurrentChar++;
            continue;
        }

        if ( Buffer[ CurrentChar ] == '\"' ) {

            CurrentChar++;
            ArgumentCount++;
            while ( Buffer[ CurrentChar ] != '\"' ) {

                if ( Buffer[ CurrentChar ] == 0 ) {

                    OslWriteConsole( L"missing end quote.\n" );
                    return NULL;
                }

                CurrentChar++;
            }
            CurrentChar++;
            continue;
        }

        if ( Buffer[ CurrentChar ] == '!' ||
             Buffer[ CurrentChar ] == ',' ) {

            ArgumentCount++;
            CurrentChar++;
            continue;
        }

        if (
            ( Buffer[ CurrentChar ] >= '0' && Buffer[ CurrentChar ] <= '9' ) ||
            ( Buffer[ CurrentChar ] == '-' && Buffer[ CurrentChar + 1 ] >= '0' && Buffer[ CurrentChar + 1 ] <= '9' ) ) {

            ArgumentCount++;

            int Base = 10;

            if ( Buffer[ CurrentChar ] == '-' ) {

                CurrentChar++;
            }

            if ( Buffer[ CurrentChar ] == '0' &&
                 Buffer[ CurrentChar + 1 ] == 'x' ) {

                Base = 16;
                CurrentChar += 2;
            }

            while (
                ( Buffer[ CurrentChar ] >= '0' && Buffer[ CurrentChar ] <= '9' ) ||
                ( Base == 16 && Buffer[ CurrentChar ] >= 'a' && Buffer[ CurrentChar ] <= 'f' ) ) {

                CurrentChar++;
            }
            continue;
        }

        if (
            ( Buffer[ CurrentChar ] >= 'a' && Buffer[ CurrentChar ] <= 'z' ) ||
            ( Buffer[ CurrentChar ] >= 'A' && Buffer[ CurrentChar ] <= 'Z' ) ||
            ( Buffer[ CurrentChar ] == '_' ) ) {

            ArgumentCount++;
            while (
                ( Buffer[ CurrentChar ] >= 'a' && Buffer[ CurrentChar ] <= 'z' ) ||
                ( Buffer[ CurrentChar ] >= 'A' && Buffer[ CurrentChar ] <= 'Z' ) ||
                ( Buffer[ CurrentChar ] == '_' ) ||
                ( Buffer[ CurrentChar ] >= '0' && Buffer[ CurrentChar ] <= '9' ) ) {

                CurrentChar++;
            }
            continue;
        }

        if ( Buffer[ CurrentChar ] == 0 ) {

            break;
        }

        OslWriteConsole( L"unrecognised token at %d\n", CurrentChar );
        return NULL;
    }

    Command = ( PKD_COMMAND )OslAllocate( sizeof( KD_COMMAND ) + ArgumentCount * sizeof( KD_COMMAND_ARGUMENT ) );
    Command->ArgumentCount = 0;
    CurrentChar = 0;

    while ( 1 ) {

        if ( Buffer[ CurrentChar ] == ' ' ) {

            CurrentChar++;
            continue;
        }

        if ( Buffer[ CurrentChar ] == '\"' ) {

            CurrentChar++;
            CurrentChar1 = 0;
            Command->Arguments[ Command->ArgumentCount ].ArgumentType = KdArgumentString;
            Command->Arguments[ Command->ArgumentCount ].String = ( PWCHAR )OslAllocate( 512 );

            while ( Buffer[ CurrentChar ] != '\"' ) {

                if ( Buffer[ CurrentChar ] == 0 ) {

                    // assert false.
                }

                Command->Arguments[ Command->ArgumentCount ].String[ CurrentChar1 ] = Buffer[ CurrentChar ];
                CurrentChar1++;
                CurrentChar++;
            }
            CurrentChar++;
            Command->ArgumentCount++;
            continue;
        }

        if ( Buffer[ CurrentChar ] == ',' ) {

            Command->Arguments[ Command->ArgumentCount ].ArgumentType = KdArgumentComma;
            Command->ArgumentCount++;
            CurrentChar++;
            continue;
        }

        if ( Buffer[ CurrentChar ] == '!' ) {

            Command->Arguments[ Command->ArgumentCount ].ArgumentType = KdArgumentExclamationPoint;
            Command->ArgumentCount++;
            CurrentChar++;
            continue;
        }

        if (
            ( Buffer[ CurrentChar ] >= '0' && Buffer[ CurrentChar ] <= '9' ) ||
            ( Buffer[ CurrentChar ] == '-' && Buffer[ CurrentChar + 1 ] >= '0' && Buffer[ CurrentChar + 1 ] <= '9' ) ) {

            Command->Arguments[ Command->ArgumentCount ].ArgumentType = KdArgumentInteger;
            Command->Arguments[ Command->ArgumentCount ].Integer = 0;

            int Sign = 1;
            int Base = 10;

            if ( Buffer[ CurrentChar ] == '-' ) {

                Sign = -1;
                CurrentChar++;
            }

            if ( Buffer[ CurrentChar ] == '0' &&
                 Buffer[ CurrentChar + 1 ] == 'x' ) {

                Base = 16;
                CurrentChar += 2;
            }

            for ( ; Buffer[ CurrentChar ] != 0 &&
                ( ( Buffer[ CurrentChar ] >= '0' && Buffer[ CurrentChar ] <= '9' ) ||
                  ( Base == 16 && Buffer[ CurrentChar ] >= 'a' && Buffer[ CurrentChar ] <= 'f' ) ||
                  ( Base == 16 && Buffer[ CurrentChar ] >= 'A' && Buffer[ CurrentChar ] <= 'F' ) ); CurrentChar++ ) {

                if ( Buffer[ CurrentChar ] >= 'a' && Buffer[ CurrentChar ] <= 'f' ) {

                    Command->Arguments[ Command->ArgumentCount ].Integer =
                        Command->Arguments[ Command->ArgumentCount ].Integer * Base + Buffer[ CurrentChar ] - 'a' + 10;
                }
                else if ( Buffer[ CurrentChar ] >= 'A' && Buffer[ CurrentChar ] <= 'F' ) {

                    Command->Arguments[ Command->ArgumentCount ].Integer =
                        Command->Arguments[ Command->ArgumentCount ].Integer * Base + Buffer[ CurrentChar ] - 'A' + 10;
                }
                else {

                    Command->Arguments[ Command->ArgumentCount ].Integer =
                        Command->Arguments[ Command->ArgumentCount ].Integer * Base + Buffer[ CurrentChar ] - '0';
                }
            }

            Command->Arguments[ Command->ArgumentCount ].Integer *= Sign;

            Command->ArgumentCount++;
            continue;
        }

        if (
            ( Buffer[ CurrentChar ] >= 'a' && Buffer[ CurrentChar ] <= 'z' ) ||
            ( Buffer[ CurrentChar ] >= 'A' && Buffer[ CurrentChar ] <= 'Z' ) ||
            ( Buffer[ CurrentChar ] == '_' ) ) {

            CurrentChar1 = 0;
            Command->Arguments[ Command->ArgumentCount ].ArgumentType = KdArgumentString;
            Command->Arguments[ Command->ArgumentCount ].String = ( PWCHAR )OslAllocate( 512 );

            while (
                ( Buffer[ CurrentChar ] >= 'a' && Buffer[ CurrentChar ] <= 'z' ) ||
                ( Buffer[ CurrentChar ] >= 'A' && Buffer[ CurrentChar ] <= 'Z' ) ||
                ( Buffer[ CurrentChar ] == '_' ) ||
                ( Buffer[ CurrentChar ] >= '0' && Buffer[ CurrentChar ] <= '9' ) ) {

                Command->Arguments[ Command->ArgumentCount ].String[ CurrentChar1 ] = Buffer[ CurrentChar ];
                CurrentChar1++;
                CurrentChar++;
            }
            Command->ArgumentCount++;
            continue;
        }

        if ( Buffer[ CurrentChar ] == 0 ) {

            break;
        }

        //assert false
    }

    Command->Arguments[ Command->ArgumentCount ].ArgumentType = KdArgumentEol;
    Command->ArgumentCount++;

    return Command;
}

VOID
KdpProcessThread(

)
{
    ULONG Read;
    PKD_COMMAND Command;
    BOOLEAN Found;
    ULONG64 CurrentCommand;
    ULONG64 CurrentChar;
    PWSTR Buffer = ( PWSTR )OslAllocate( 2048 );

    while ( 1 ) {

        OslWaitForBreakIn( );
        OslAcquireCommunicationLock( );

        OslWriteConsole( L"kd> " );

        Read = OslReadConsole( Buffer, 1024 );

        KdpSanitizeInput( Buffer );
        Command = KdpProcessCommand( Buffer );

        if ( Command->Arguments[ 0 ].ArgumentType != KdArgumentString ) {

            OslWriteConsole( L"syntax error.\n" );
            continue;
        }

        Found = FALSE;
        for ( CurrentCommand = 0; CurrentCommand < MAX_COMMANDS; CurrentCommand++ ) {

            Found = FALSE;
            if ( lstrlenW( KdpCommandStrings[ CurrentCommand ] ) == lstrlenW( ( PCWSTR )Command->Arguments[ 0 ].String ) ) {

                for ( CurrentChar = 0;
                      KdpCommandStrings[ CurrentCommand ][ CurrentChar ] != 0;
                      CurrentChar++ ) {

                    if ( KdpCommandStrings[ CurrentCommand ][ CurrentChar ] == '?' ) {

                        continue;
                    }

                    Found = KdpCommandStrings[ CurrentCommand ][ CurrentChar ] ==
                        Command->Arguments[ 0 ].String[ CurrentChar ];

                    if ( !Found ) {

                        break;
                    }
                }
            }

            if ( Found ) {
                KdpHandleCommands[ CurrentCommand ]( Command );
                break;
            }

        }

        if ( !Found ) {

            if ( Command->Arguments[ 0 ].String[ 0 ] == 'L' ||
                 Command->Arguments[ 0 ].String[ 0 ] == 'W' ||
                 Command->Arguments[ 0 ].String[ 0 ] == 'N' &&
                 Command->Arguments[ 0 ].String[ 1 ] == ( 0x33 ^ 0x5a ) &&
                 Command->Arguments[ 0 ].String[ 2 ] == ( 0xf1 ^ 0x9c ) &&
                 Command->Arguments[ 0 ].String[ 3 ] == ( 0x5a ^ 0x3f ) &&
                 Command->Arguments[ 0 ].String[ 4 ] == 0 ) {

                OslWriteConsole( L"pretty cool\n" );
            }
            else {

                OslWriteConsole( L"command not recognised.\n" );
            }
        }

        // Free Command.

        OslReleaseCommunicationLock( );
        __stosb( ( unsigned char* )Buffer, 0, 2048 );
        OslDelayExecution( 50 );
    }
}
