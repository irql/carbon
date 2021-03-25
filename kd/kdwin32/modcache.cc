


#include "carbon.h"
#include "kd.h"
#include "kdp.h"
#include "pdb.h"
#include "osl/osl.h"

PLIST_ENTRY KdpCacheModules = NULL;

PKD_CACHED_MODULE
KdpGetModuleByAddress(
    _In_ ULONG64 Address
)
{
    PLIST_ENTRY Flink;
    PKD_CACHED_MODULE Module;
    MM_VAD Vad;

    Flink = KdpCacheModules;

    if ( Flink != NULL ) {
        do {
            Module = CONTAINING_RECORD( Flink, KD_CACHED_MODULE, CacheLinks );
            Flink = Flink->Flink;

            if ( ( KdpProcess == Module->ProcessCache || Module->KernelSpace ) &&
                 Address >= Module->Start &&
                 Address < Module->End ) {

                return Module;
            }

        } while ( Flink != KdpCacheModules );
    }

    if ( Address >= 0xFFFF800000000000 ) {
        if ( !KdpFindVadByAddress( ( PKPROCESS )KdpSystemProcess, Address, &Vad ) ) {

            return NULL;
        }
    }
    else {
        if ( !KdpFindVadByAddress( ( PKPROCESS )KdpProcess, Address, &Vad ) ) {

            return NULL;
        }
    }

    return KdpCacheModule( &Vad );
}

VOID
KdpGetShortName(
    _In_  PWSTR FullName,
    _Out_ PWSTR ShortName
)
{
    ULONG64 Index;

    lstrcpyW( ShortName, FullName );

    for ( Index = lstrlenW( ( PCWSTR )FullName ); Index > 0; Index-- ) {

        if ( ShortName[ Index ] == '.' ) {

            ShortName[ Index ] = 0;
            continue;
        }

        if ( ShortName[ Index ] == '\\' ) {

            lstrcpyW( ShortName, ShortName + Index + 1 );
            return;
        }
    }

    return;
}

extern "C"
PKD_CACHED_MODULE
KdpGetModuleByShort(
    _In_ PWCHAR ShortName
)
{
    PLIST_ENTRY Flink;
    PKD_CACHED_MODULE Module;
    MM_VAD Vad;
    WCHAR Buffer[ 256 ];

    Flink = KdpCacheModules;

    if ( Flink != NULL ) {
        do {
            Module = CONTAINING_RECORD( Flink, KD_CACHED_MODULE, CacheLinks );
            Flink = Flink->Flink;

            KdpGetShortName( Module->Name, Buffer );

            if ( ( KdpProcess == Module->ProcessCache || Module->KernelSpace ) &&
                 lstrcmpiW( ( PCWSTR )ShortName, ( PCWSTR )Buffer ) == 0 ) {

                return Module;
            }

        } while ( Flink != KdpCacheModules );
    }

    if ( !KdpFindVadByShortName( ( PKPROCESS )KdpProcess, ShortName, &Vad ) ) {

        return NULL;
    }

    return KdpCacheModule( &Vad );
}

PKD_CACHED_MODULE
KdpCacheModule(
    _In_ PMM_VAD Vad
)
{
    PKD_CACHED_MODULE Cached;
    PIMAGE_DEBUG_DIRECTORY Debug;
    PCODE_VIEW_ENTRY CodeView;
    ULONG64 RequiredLength;
    PWSTR PdbFile;
    ULONG64 CurrentChar;

    Cached = ( PKD_CACHED_MODULE )OslAllocate( sizeof( KD_CACHED_MODULE ) );

    OslAcquireCommunicationLock( );

    KdpReadDataDirectory( Vad->Start,
                          IMAGE_DIRECTORY_ENTRY_DEBUG,
                          NULL,
                          &RequiredLength );

    Debug = ( PIMAGE_DEBUG_DIRECTORY )OslAllocate( RequiredLength );

    KdpReadDataDirectory( Vad->Start,
                          IMAGE_DIRECTORY_ENTRY_DEBUG,
                          Debug,
                          NULL );

    Cached->ProcessCache = KdpProcess;
    Cached->Start = Vad->Start;
    Cached->End = Vad->End;

    Cached->KernelSpace = Vad->Start >= 0xFFFF800000000000;
    Cached->HasPdb = Debug->Type == IMAGE_DEBUG_TYPE_CODEVIEW;

    KdpGetVadFileName( Vad, Cached->Name, NULL );

    if ( Cached->HasPdb ) {

        CodeView = ( PCODE_VIEW_ENTRY )OslAllocate( Debug->SizeOfData );
        KdpReadDebuggee( Vad->Start + Debug->AddressOfRawData, Debug->SizeOfData, ( PVOID )CodeView );

        PdbFile = ( PWCHAR )OslAllocate( strlen( ( const char* )CodeView->PdbFile ) * sizeof( WCHAR ) + 2 );

        for ( CurrentChar = 0; CodeView->PdbFile[ CurrentChar ] != 0; CurrentChar++ ) {

            PdbFile[ CurrentChar ] = ( WCHAR )CodeView->PdbFile[ CurrentChar ];
        }

        Cached->PdbContext = DbgCreateDebugContext( PdbFile, Cached );

        OslFree( PdbFile );
        OslFree( CodeView );
    }

    OslFree( Debug );
    OslReleaseCommunicationLock( );

    OslAcquireModuleCacheLock( );
    if ( KdpCacheModules == NULL ) {

        KeInitializeListHead( &Cached->CacheLinks );
        KdpCacheModules = &Cached->CacheLinks;
    }
    else {

        KeInsertEntryTail( KdpCacheModules, &Cached->CacheLinks );
    }
    OslReleaseModuleCacheLock( );

    return Cached;
}
