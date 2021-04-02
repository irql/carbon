


#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <dia2.h>

#include "kd.h"
#include "pdb.h"
#include "kdp.h"
#include "osl/osl.h"

PKD_CACHED_MODULE KdpKernelContext;

HRESULT
DbgInitialize(

)
{

    //
    // The initialization code for kd is very hardcoded and broken, you should have
    // kdcom send you a control block containing various variables such as the kernel
    // base, also allow reconnects when a kd is disconnected.
    //

    //KdpKernelContext;// = ( PKPDB_CONTEXT )HeapAlloc( GetProcessHeap( ), HEAP_ZERO_MEMORY, sizeof( KPDB_CONTEXT ) );

    //DbgLoadPdb( L"I:\\repos\\osdev\\carbon_v2\\x64\\Release\\symbols\\carbkrnl.pdb",
    //            KdpKernelContext );

    KdpKernelContext = ( PKD_CACHED_MODULE )OslAllocate( sizeof( KD_CACHED_MODULE ) );//KdpGetModuleByAddress( 0xFFFFFFFFFFE00000 );
    KdpKernelContext->Start = 0xFFFFFFFFFFE00000;
    KdpKernelContext->End = 0xFFFFFFFFFFF00000;
    KdpKernelContext->HasPdb = TRUE;
    KdpKernelContext->KernelSpace = TRUE;
    KdpKernelContext->ProcessCache = NULL;
    wcscpy( KdpKernelContext->Name, L"\\SYSTEM\\CARBKRNL.SYS" );
    DbgCreateDebugContext( ( PWSTR )L"I:\\repos\\osdev\\carbon_v2\\x64\\Release\\symbols\\carbkrnl.pdb",
                           KdpKernelContext );
    return S_OK;
}

HRESULT
DbgLoadPdb(
    _In_  PKD_CACHED_MODULE Module,
    _In_  PWSTR             PdbFileName,
    _Out_ PKPDB_CONTEXT     Context
)
{
    HRESULT hResult;

    wcscpy( Context->FileName, PdbFileName );

    hResult = CoInitialize( NULL );
    hResult = CoCreateInstance( __uuidof( DiaSource ),
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                __uuidof( IDiaDataSource ),
                                ( LPVOID* )&Context->Source );
    if ( hResult != S_OK ) {

        //
        // regsvr32.exe bin/amd64/msdia140.dll
        //

        return hResult;
    }

    hResult = Context->Source->loadDataFromPdb( Context->FileName );
    hResult = Context->Source->openSession( &Context->Session );
    hResult = Context->Session->get_globalScope( &Context->Global );

    return hResult;
}

HRESULT
DbgFieldOffset(
    _In_  PKD_CACHED_MODULE Context,
    _In_  PCWSTR            TypeName,
    _In_  PCWSTR            FieldName,
    _Out_ LONG*             FieldOffset
)
{
    HRESULT hResult;
    IDiaEnumSymbols* EnumSymbols;
    IDiaEnumSymbols* EnumChildren;
    IDiaSymbol* Symbol;
    IDiaSymbol* Child;
    ULONG Celt;
    BSTR Name;

    //
    // Make this function recursive
    //

    Celt = 0;
    hResult = Context->PdbContext->Global->findChildren( SymTagUDT,
                                                         TypeName,
                                                         nsCaseInsensitive,
                                                         &EnumSymbols );

    //
    // Assume it's the first found.
    //

    hResult = EnumSymbols->Next( 1, &Symbol, &Celt );

    //
    // Take a look into get_symTag
    //

    Symbol->findChildren( SymTagNull, NULL, nsNone, &EnumChildren );

    while ( EnumChildren->Next( 1, &Child, &Celt ) == S_OK && Celt == 1 ) {

        Child->get_name( &Name );
        Child->get_offset( FieldOffset );

        if ( lstrcmpiW( Name, FieldName ) == 0 ) {

            Child->Release( );
            return S_OK;
        }

        Child->Release( );
    }

    Symbol->Release( );
    EnumSymbols->Release( );

    return ( HRESULT )-1;
}

VOID
DbgFreeString(
    _In_ PWCHAR String
)
{
    SysFreeString( String );
}

extern "C"
HRESULT
DbgGetFunctionByAddress(
    _In_  PKD_CACHED_MODULE Context,
    _In_  ULONG64           Address,
    _Out_ PWCHAR*           FunctionName,
    _Out_ PWCHAR*           FileName,
    _Out_ PULONG32          LineNumber,
    _Out_ PLONG             FunctionDisplacement
)
{

    //
    // Look into PrintLines
    //

    HRESULT hResult;
    IDiaEnumLineNumbers* LineNumbers;
    IDiaLineNumber* Line;
    IDiaSourceFile* SourceFile;
    IDiaSymbol* Symbol;
    ULONG Celt;

    hResult = Context->PdbContext->Session->findSymbolByRVAEx( ( DWORD )Address,
                                                               SymTagNull,
                                                               &Symbol,
                                                               FunctionDisplacement );
    if ( hResult != S_OK ) {

        return hResult;
    }

    hResult = Symbol->get_name( FunctionName );
    if ( hResult != S_OK ) {

        Symbol->Release( );
        return hResult;
    }

    hResult = Context->PdbContext->Session->findLinesByRVA( ( DWORD )Address,
                                                            1,
                                                            &LineNumbers );
    if ( hResult != S_OK ) {

        SysFreeString( *FunctionName );
        Symbol->Release( );
        return hResult;
    }

    hResult = LineNumbers->Next( 1, &Line, &Celt );
    if ( hResult != S_OK || Celt != 1 ) {

        LineNumbers->Release( );
        SysFreeString( *FunctionName );
        Symbol->Release( );
        return hResult;
    }

    hResult = Line->get_sourceFile( &SourceFile );
    if ( hResult != S_OK ) {

        Line->Release( );
        LineNumbers->Release( );
        SysFreeString( *FunctionName );
        Symbol->Release( );
        return hResult;
    }

    hResult = Line->get_lineNumber( ( PDWORD )LineNumber );
    if ( hResult != S_OK ) {

        SourceFile->Release( );
        Line->Release( );
        LineNumbers->Release( );
        SysFreeString( *FunctionName );
        Symbol->Release( );
        return hResult;
    }
    SourceFile->get_fileName( FileName );

    return S_OK;
}

extern "C"
PKPDB_CONTEXT
DbgCreateDebugContext(
    _In_ PWSTR             FileName,
    _In_ PKD_CACHED_MODULE Cached
)
{

    //
    // This should manage symbols and keep track of where everything is.
    //

    PKPDB_CONTEXT Context = ( PKPDB_CONTEXT )OslAllocate( sizeof( KPDB_CONTEXT ) );

    DbgLoadPdb( Cached, FileName, Context );

    Cached->PdbContext = Context;
    Context->Cached = Cached;

    return Context;
}

//
// Returns an rva.
//

HRESULT
DbgGetAddressByName(
    _In_  PKD_CACHED_MODULE Context,
    _In_  PCWSTR            Name,
    _Out_ ULONG*            Rva
)
{
    HRESULT hResult;
    IDiaEnumSymbols* EnumSymbols;
    IDiaSymbol* Symbol;
    ULONG Celt;
    ULONG LocationType;

    Celt = 0;
    hResult = Context->PdbContext->Global->findChildren( SymTagData,
                                                         Name,
                                                         nsCaseInRegularExpression,
                                                         &EnumSymbols );
    if ( hResult != S_OK ) {

        return hResult;
    }

    hResult = EnumSymbols->Next( 1, &Symbol, &Celt );

    if ( hResult != S_OK || Celt != 1 ) {

        EnumSymbols->Release( );
        return hResult;
    }

    if ( Symbol->get_locationType( &LocationType ) == S_OK ) {

        switch ( LocationType ) {
        case LocIsStatic:
        case LocIsTLS:
        case LocInMetaData:
        case LocIsIlRel:

            return Symbol->get_relativeVirtualAddress( Rva );
        default:
            break;
        }
    }

    return ( HRESULT )-1;
}

HRESULT
DbgGetFunctionByName(
    _In_  PKD_CACHED_MODULE Context,
    _In_  PCWSTR            Name,
    _Out_ ULONG*            Rva
)
{
    HRESULT hResult;
    IDiaEnumSymbols* EnumSymbols;
    IDiaSymbol* Symbol;
    ULONG Celt;
    ULONG LocationType;

    Celt = 0;
    hResult = Context->PdbContext->Global->findChildren( SymTagFunction,
                                                         Name,
                                                         nsCaseInRegularExpression,
                                                         &EnumSymbols );
    if ( hResult != S_OK ) {

        return hResult;
    }

    hResult = EnumSymbols->Next( 1, &Symbol, &Celt );

    if ( hResult != S_OK || Celt != 1 ) {

        EnumSymbols->Release( );
        return hResult;
    }

    if ( Symbol->get_locationType( &LocationType ) == S_OK ) {

        switch ( LocationType ) {
        case LocIsStatic:
        case LocIsTLS:
        case LocInMetaData:
        case LocIsIlRel:

            return Symbol->get_relativeVirtualAddress( Rva );
        default:
            break;
        }
    }

    return ( HRESULT )-1;
}

EXTERN_C
HRESULT
DbgPrintFunctionFrame(
    _In_ PKD_CACHED_MODULE Context,
    _In_ PWCHAR            FunctionName,
    _In_ ULONG64           FrameBase
)
{
    FrameBase;
    HRESULT hResult;
    IDiaEnumSymbols* EnumSymbols;
    IDiaSymbol* FunctionSymbol;
    IDiaSymbol* DataSymbol;
    ULONG Celt;
    ULONG DataKind;
    LONG AddressOffset;

    Celt = 0;
    hResult = Context->PdbContext->Global->findChildren( SymTagFunction,
                                                         FunctionName,
                                                         nsCaseInRegularExpression,
                                                         &EnumSymbols );
    if ( hResult != S_OK ) {

        return hResult;
    }

    hResult = EnumSymbols->Next( 1, &FunctionSymbol, &Celt );
    EnumSymbols->Release( );

    if ( hResult != S_OK || Celt != 1 ) {

        return hResult;
    }

    //
    // FunctionSymbol is now the function symbol.
    //

    FunctionSymbol->findChildren( SymTagData,
                                  NULL,
                                  nsNone,
                                  &EnumSymbols );

    while ( EnumSymbols->Next( 1, &DataSymbol, &Celt ) == S_OK && Celt == 1 ) {

        DataSymbol->get_dataKind( &DataKind );

        switch ( DataKind ) {
        case DataIsLocal:
        case DataIsParam:

            BSTR p;
            DataSymbol->get_name( &p );
            DataSymbol->get_offset( &AddressOffset );
            OslWriteConsole( L"  %30s (0x%p) - 0x%p\n",
                             p, FrameBase + AddressOffset, KdpReadULong64( FrameBase + AddressOffset ) );
            break;
        default:
            break;
        }

        DataSymbol->Release( );
    }

    return S_OK;
}
