


#include <carbsup.h>
#include "ldrp.h"
#include "../../mm/mi.h"
#include "../../hal/halp.h"
#include "../../ke/ki.h"

//
// This macro is the default directory for searching for dll's when
// resolving import tables for supervisor modules.
//

#define DEFAULT_DIRECTORY L"\\SYSTEM\\"

NTSTATUS
LdrpGetLoaderLimits(
    _In_    PVOID    FileLoaded,
    _Inout_ PULONG64 ModuleLength
)
{
    PIMAGE_DOS_HEADER HeaderDos;
    PIMAGE_NT_HEADERS HeadersNt;
    PIMAGE_SECTION_HEADER LastSection;

    HeaderDos = ( PIMAGE_DOS_HEADER )( FileLoaded );
    if ( !LdrpCheckDos( HeaderDos ) ) {

        return STATUS_INVALID_IMAGE;
    }

    HeadersNt = ( PIMAGE_NT_HEADERS )( ( PUCHAR )FileLoaded + HeaderDos->e_lfanew );
    if ( !LdrpCheckNt( HeadersNt ) ) {

        return STATUS_INVALID_IMAGE;
    }

    LastSection = &IMAGE_FIRST_SECTION( HeadersNt )[ HeadersNt->FileHeader.NumberOfSections - 1 ];
    *ModuleLength = ( ULONG64 )LastSection->VirtualAddress + ROUND_TO_PAGES( LastSection->Misc.VirtualSize );
    return STATUS_SUCCESS;
}

NTSTATUS
LdrpLoadSupervisorModule(
    _In_ PKPROCESS Process,
    _In_ PMM_VAD   Vad,
    _In_ PVOID     LoadBase,
    _In_ ULONG64   LoadLength,
    _In_ PVOID     FileBase,
    _In_ ULONG64   FileLength
)
{
    Process;
    FileLength;

    NTSTATUS ntStatus;
    PIMAGE_DOS_HEADER HeaderDos;
    PIMAGE_NT_HEADERS HeadersNt;
    PIMAGE_SECTION_HEADER HeadersSection;
    PIMAGE_IMPORT_DESCRIPTOR Import;
    USHORT CurrentSection;
    ULONG64 Char;
    UNICODE_STRING FileName;
    PMM_VAD CurrentVad;
    OBJECT_ATTRIBUTES ObjectAttributes = { RTL_CONSTANT_STRING( L"\\??\\BootDevice" ) };

    ntStatus = STATUS_SUCCESS;
    Vad->Start = 0;
    HeaderDos = ( PIMAGE_DOS_HEADER )( FileBase );
    if ( !LdrpCheckDos( HeaderDos ) ) {

        return STATUS_INVALID_IMAGE;
    }

    HeadersNt = ( PIMAGE_NT_HEADERS )( ( PUCHAR )FileBase + HeaderDos->e_lfanew );
    if ( !LdrpCheckNt( HeadersNt ) ) {

        return STATUS_INVALID_IMAGE;
    }

    HeadersSection = IMAGE_FIRST_SECTION( HeadersNt );
    Vad->Charge = LoadLength;
    Vad->Start = ( ULONG64 )LoadBase;
    Vad->End = Vad->Start + Vad->Charge;

    RtlCopyMemory( LoadBase, FileBase, HeadersNt->OptionalHeader.SizeOfHeaders );

    for ( CurrentSection = 0; CurrentSection < HeadersNt->FileHeader.NumberOfSections; CurrentSection++ ) {


        RtlCopyMemory(
            ( PUCHAR )LoadBase + HeadersSection[ CurrentSection ].VirtualAddress,
            ( PUCHAR )FileBase + HeadersSection[ CurrentSection ].PointerToRawData,
            HeadersSection[ CurrentSection ].SizeOfRawData );
    }

    if ( LdrpIsRelocImage( HeadersNt ) &&
         HeadersNt->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_BASERELOC ].VirtualAddress != 0 &&
         LoadBase != ( PVOID )HeadersNt->OptionalHeader.ImageBase ) {

        ntStatus = LdrResolveBaseReloc( LoadBase );

        if ( !NT_SUCCESS( ntStatus ) ) {

            return ntStatus;
        }
    }

    MiInsertVad( Process, Vad );

    if ( HeadersNt->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_IMPORT ].VirtualAddress != 0 ) {

        Import = ( PIMAGE_IMPORT_DESCRIPTOR )( ( PUCHAR )LoadBase +
                                               HeadersNt->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_IMPORT ].VirtualAddress );
        RTL_STACK_STRING( FileName, 256 );
        RTL_STACK_STRING( ObjectAttributes.RootDirectory, 256 );

        while ( Import->Characteristics ) {

            if ( !NT_SUCCESS( ntStatus ) ) {

                return ntStatus;
            }

            for ( Char = 0; ( ( PCHAR )Vad->Start + Import->Name )[ Char ] != 0; Char++ ) {

                FileName.Buffer[ Char ] = ( ( PCHAR )Vad->Start + Import->Name )[ Char ];
                FileName.Buffer[ Char + 1 ] = 0;
            }

            FileName.Length = ( USHORT )RtlStringLength( FileName.Buffer ) * sizeof( WCHAR );

            CurrentVad = MiFindVadByShortName( Process, &FileName );

            if ( CurrentVad != NULL ) {

                //
                // In the current state of the MM_VAD system,
                // import's are resolved by truncating other vad's
                // FileName's (taken from FileObject) down to their root
                // file name and compared against the short name.
                //
                // Loading \SYSTEM\CARBKRNL.SYS and then
                // \SYSTEM\EXAMPLE\CARBKRNL.SYS will both
                // have the same short name, meaning the first
                // in the processes VadRoot will take priority.
                //
                // ShortNames are compared case insensitively.
                //

                //RtlDebugPrint( L"I love my greasy pogger %s %ull\n", FileName.Buffer, CurrentVad->Start );
                ntStatus = LdrResolveImportTable( LoadBase, ( PVOID )CurrentVad->Start, Import );

                Import++;
                continue;
            }
            else {

                lstrcpyW( ObjectAttributes.RootDirectory.Buffer, DEFAULT_DIRECTORY );
                lstrcatW( ObjectAttributes.RootDirectory.Buffer, FileName.Buffer ); // TODO: potentially buffer overflow, be careful w strings.


                //
                // This needs to be fixed
                //

                //RtlDebugPrint( L"I want to load %s\n", ObjectAttributes.RootDirectory.Buffer );

                //while ( 1 );
                __debugbreak( );
            }
            //RtlDebugPrint( L"Yeapin eck\n." );
        }

        //RtlDebugPrint( L"ppp-p-p-p-p-pog..\n" );
    }

    return STATUS_SUCCESS;
}
