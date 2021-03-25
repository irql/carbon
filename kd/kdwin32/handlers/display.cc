


#include <windows.h>
#include <dia2.h>
#include "../kd.h"
#include "../pdb.h"
#include "../osl/osl.h"

//
// TODO: Move to dbg/pdb.cc as this is part of the 
// osl
//

HRESULT
KdpPrintSymbolType(
    _In_ IDiaSymbol* Symbol
)
{
    HRESULT hResult;
    ULONG SymbolTag;
    BSTR TypeName;
    BOOL Value;
    ULONG64 Length;
    IDiaSymbol* PointerType;

    hResult = Symbol->get_symTag( &SymbolTag );
    if ( FAILED( hResult ) ) {

        return hResult;
    }

    if ( FAILED( Symbol->get_name( &TypeName ) ) ) {

        TypeName = NULL;
    }

    if ( SymbolTag != SymTagPointerType ) {

        if ( SUCCEEDED( Symbol->get_constType( &Value ) ) && Value ) {

            OslWriteConsole( L"CONST " );
        }

        if ( SUCCEEDED( Symbol->get_volatileType( &Value ) ) && Value ) {

            OslWriteConsole( L"VOLATILE " );
        }

        if ( SUCCEEDED( Symbol->get_unalignedType( &Value ) ) && Value ) {

            OslWriteConsole( L"UNALIGNED " );
        }
    }

    hResult = Symbol->get_length( &Length );
    if ( FAILED( hResult ) ) {

        return hResult;
    }

    switch ( SymbolTag ) {
    case SymTagUDT:
        OslWriteConsole( L"%s", TypeName );
        break;
    case SymTagEnum:
        OslWriteConsole( L"enum %s", TypeName );
        break;
    case SymTagFunctionType:
        OslWriteConsole( L"FARPROC" );
        break;
    case SymTagPointerType:
        Symbol->get_type( &PointerType );
        KdpPrintSymbolType( PointerType );
        OslWriteConsole( L"*" );
        break;
    case SymTagBaseType:
        //Symbol->get_baseType( );
        if ( Length == 0 ) {

            OslWriteConsole( L"VOID" );
        }
        else {

            OslWriteConsole( L"ULONG%dB", Length );
        }
        break;
    default:
        OslWriteConsole( L"<unknown-type>" );
        break;
    }

    if ( TypeName != NULL ) {

        SysFreeString( TypeName );
    }

    return S_OK;
}

HRESULT
KdpPrintSymbolByName(
    _In_ PKD_CACHED_MODULE Context,
    _In_ PWSTR             Name
)
{
    HRESULT hResult;
    IDiaEnumSymbols* EnumSymbols;
    IDiaEnumSymbols* EnumChildren;
    IDiaSymbol* Symbol;
    IDiaSymbol* ChildSymbol;
    IDiaSymbol* TypeSymbol;
    ULONG Celt;
    ULONG SymbolTag;

    BSTR FieldName;
    LONG FieldOffset;

    ULONG LocationType;
    ULONG Rva;

    Celt = 0;
    hResult = Context->PdbContext->Global->findChildren( SymTagNull,
                                                         Name,
                                                         nsCaseInRegularExpression,
                                                         &EnumSymbols );
    if ( hResult != S_OK ) {

        return hResult;
    }

    hResult = EnumSymbols->Next( 1, &Symbol, &Celt );

    if ( hResult != S_OK ) {

        EnumSymbols->Release( );
        return hResult;
    }

    Symbol->get_symTag( &SymbolTag );

    switch ( SymbolTag ) {
    case SymTagUDT:
        Symbol->findChildren( SymTagNull, NULL, nsNone, &EnumChildren );

        while ( EnumChildren->Next( 1, &ChildSymbol, &Celt ) == S_OK && Celt == 1 ) {

            ChildSymbol->get_name( &FieldName );
            ChildSymbol->get_offset( &FieldOffset );

            OslWriteConsole( L"  [+%#04x] %20s ", FieldOffset, FieldName );

            if ( SUCCEEDED( ChildSymbol->get_type( &TypeSymbol ) ) ) {

                KdpPrintSymbolType( TypeSymbol );
            }

            OslWriteConsole( L"\n" );

            SysFreeString( FieldName );

            ChildSymbol->Release( );
        }

        break;
    case SymTagData:

        if ( SUCCEEDED( Symbol->get_locationType( &LocationType ) ) ) {

            switch ( LocationType ) {
            case LocIsStatic:
            case LocIsTLS:
            case LocInMetaData:
            case LocIsIlRel:
                if ( Symbol->get_relativeVirtualAddress( &Rva ) == S_OK ) {

                    OslWriteConsole( L"  %20s : 0x%p\n", Name, Context->Start + Rva );
                }

            default:
                break;
            }
        }

        break;
    default:
        OslWriteConsole( L"unrecognised symbol tag: %d\n", SymbolTag );
        break;
    }

    Symbol->Release( );
    EnumSymbols->Release( );

    return S_OK;
}

VOID
KdpHandleDisplay(
    _In_ PKD_COMMAND Command
)
{
    //
    // Currently only works for UDTs
    //

    PKD_CACHED_MODULE Module;
    ULONG64( *KdpRead )( ULONG64 );
    PCWSTR Format;
    ULONG Rva;
    ULONG64 CurrentByte;
    ULONG64 Length;

    Format = NULL;
    KdpRead = NULL;

    //
    // When you can be bothered, optimize the reading of variables to 
    // read it into a buffer and display it afterwards :gorilla:
    //

    switch ( Command->Arguments[ 0 ].String[ 1 ] ) {
    case 't': // Display Type

        if ( Command->ArgumentCount != 5 ||
             Command->Arguments[ 1 ].ArgumentType != KdArgumentString ||
             Command->Arguments[ 2 ].ArgumentType != KdArgumentExclamationPoint ||
             Command->Arguments[ 3 ].ArgumentType != KdArgumentString ||
             Command->Arguments[ 4 ].ArgumentType != KdArgumentEol ) {

            OslWriteConsole( L"syntax error.\n" );
            return;
        }

        Module = KdpGetModuleByShort( Command->Arguments[ 1 ].String );

        if ( Module != NULL ) {

            if ( FAILED( KdpPrintSymbolByName( Module, Command->Arguments[ 3 ].String ) ) ) {

                OslWriteConsole( L"failed.\n" );
            }
        }

        break; // can you clean this up lol
    case 'q': // ulong64
        Format = L"  0x%p";
        KdpRead = ( decltype( KdpRead ) )KdpReadULong64;
        Length = 8;
    case 'd': // ulong32
        if ( KdpRead == NULL ) {
            Format = L"  0x%08x";
            KdpRead = ( decltype( KdpRead ) )KdpReadULong32;
            Length = 4;
        }
    case 's': // short
    case 'w': // word
        if ( KdpRead == NULL ) {
            Format = L"  0x%04x";
            KdpRead = ( decltype( KdpRead ) )KdpReadUShort;
            Length = 2;
        }
    case 'c': // char
    case 'b': // byte
        if ( KdpRead == NULL ) {
            Format = L"  0x%02x";
            KdpRead = ( decltype( KdpRead ) )KdpReadUChar;
            Length = 1;
        }

        if ( Command->ArgumentCount == 3 &&
             Command->Arguments[ 1 ].ArgumentType == KdArgumentInteger &&
             Command->Arguments[ 2 ].ArgumentType == KdArgumentEol ) {

            OslWriteConsole( Format, KdpRead( Command->Arguments[ 1 ].Integer ) );
            OslWriteConsole( L"\n" );
        }
        else if ( Command->ArgumentCount == 4 &&
                  Command->Arguments[ 1 ].ArgumentType == KdArgumentInteger &&
                  Command->Arguments[ 2 ].ArgumentType == KdArgumentInteger &&
                  Command->Arguments[ 3 ].ArgumentType == KdArgumentEol ) {

            CurrentByte = 0;
            while ( CurrentByte < ( ULONG64 )Command->Arguments[ 2 ].Integer ) {
                OslWriteConsole( Format, KdpRead( Command->Arguments[ 1 ].Integer ) );
                Command->Arguments[ 1 ].Integer += Length;
                CurrentByte++;
                if ( ( CurrentByte % 16 ) == 0 ) {
                    OslWriteConsole( L"\n" );
                }
            }

            if ( ( CurrentByte % 16 ) != 0 ) {

                OslWriteConsole( L"\n" );
            }
        }
        else if ( Command->ArgumentCount == 5 &&
                  Command->Arguments[ 1 ].ArgumentType == KdArgumentString &&
                  Command->Arguments[ 2 ].ArgumentType == KdArgumentExclamationPoint &&
                  Command->Arguments[ 3 ].ArgumentType == KdArgumentString &&
                  Command->Arguments[ 4 ].ArgumentType == KdArgumentEol ) {

            Module = KdpGetModuleByShort( Command->Arguments[ 1 ].String );

            if ( Module != NULL ) {

                if ( DbgGetAddressByName( Module, Command->Arguments[ 3 ].String, &Rva ) == S_OK ) {

                    OslWriteConsole( Format, KdpRead( Module->Start + Rva ) );
                    OslWriteConsole( L"\n" );
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
