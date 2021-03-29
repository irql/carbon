


#include <carbsup.h>
#include "ldrp.h"
#include "../../mm/mi.h"
#include "../../hal/halp.h"
#include "../../ke/ki.h"

NTSTATUS
LdrpLoadUserModuleEx(
    _Out_ PHANDLE SectionHandle,
    _In_  PWSTR   FileName
)
{
    //
    // FileName is something like "ntdll.dll". This function deals
    // with \KnownDlls\ searching.
    //
    // This is a pretty ass-blast function, just wraps a lot of other functions
    // which call into deeper ldrp apis.
    //

    NTSTATUS ntStatus;
    HANDLE FileHandle;
    IO_STATUS_BLOCK StatusBlock;
    OBJECT_ATTRIBUTES File = { RTL_CONSTANT_STRING( L"\\??\\BootDevice" ) };
    OBJECT_ATTRIBUTES Section = { 0 };
    UNICODE_STRING KnownPath;
    PMM_SECTION_OBJECT SectionObject;

    RTL_STACK_STRING( KnownPath, 256 );

    //RtlDebugPrint( L"%s\n", FileName );

    SectionObject = NULL;
    *SectionHandle = 0;
    FileHandle = 0;

    lstrcpyW( KnownPath.Buffer, L"\\KnownDlls\\" );
    lstrcatW( KnownPath.Buffer, FileName );
    KnownPath.Length = ( USHORT )lstrlenW( KnownPath.Buffer ) * sizeof( WCHAR );

    ntStatus = ObReferenceObjectByName( &SectionObject,
                                        &KnownPath,
                                        MmSectionObject );
    if ( !NT_SUCCESS( ntStatus ) ) {

        //
        // Not a known dll
        //

        RTL_STACK_STRING( File.RootDirectory, 256 );
        lstrcpyW( File.RootDirectory.Buffer, L"\\SYSTEM\\" );
        lstrcatW( File.RootDirectory.Buffer, FileName );
        File.RootDirectory.Length = ( USHORT )lstrlenW( File.RootDirectory.Buffer ) * sizeof( WCHAR );

        ntStatus = ZwCreateFile( &FileHandle,
                                 &StatusBlock,
                                 GENERIC_ALL | SYNCHRONIZE,
                                 &File,
                                 FILE_OPEN_IF,
                                 FILE_SHARE_READ,
                                 0 );
        if ( !NT_SUCCESS( ntStatus ) ||
             !NT_SUCCESS( StatusBlock.Status ) ) {

            goto LdrpProcedureFinished;
        }

        ntStatus = ZwCreateSection( SectionHandle,
                                    SECTION_ALL_ACCESS,
                                    &Section,
                                    SEC_EXECUTE | SEC_WRITE | SEC_IMAGE,
                                    FileHandle );
    }
    else {

        //
        // KnownDll
        //

        ntStatus = ObOpenObjectFromPointer( SectionHandle,
                                            SectionObject,
                                            SECTION_ALL_ACCESS,
                                            OBJ_KERNEL_HANDLE,
                                            KernelMode );
        ObDereferenceObject( SectionObject );;
    }

LdrpProcedureFinished:;

    if ( FileHandle != 0 ) {

        ZwClose( FileHandle );
    }

    return ntStatus;
}

NTSTATUS
LdrpLoadUserModule(
    _In_ PKPROCESS Process,
    _In_ PMM_VAD   Vad,
    _In_ PVOID     LoadBase,
    _In_ ULONG64   LoadLength,
    _In_ PVOID     FileBase,
    _In_ ULONG64   FileLength
)
{
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
    HANDLE SectionHandle;
    HANDLE ProcessHandle;
    PVOID BaseAddress;

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

    //
    // Poggers? this is so that if an imported module wants to import
    // a function from this module, it can do when resolving the poggers.
    //

    MiInsertVad( Process, Vad );

    if ( HeadersNt->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_IMPORT ].VirtualAddress != 0 ) {

        Import = ( PIMAGE_IMPORT_DESCRIPTOR )( ( PUCHAR )LoadBase +
                                               HeadersNt->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_IMPORT ].VirtualAddress );
        RTL_STACK_STRING( FileName, 256 );

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

                ntStatus = LdrResolveImportTable( LoadBase, ( PVOID )CurrentVad->Start, Import );

                Import++;
                continue;
            }
            else {

                // should never fail lol
                ntStatus = ObOpenObjectFromPointer( &ProcessHandle,
                                                    Process,
                                                    PROCESS_ALL_ACCESS,
                                                    OBJ_KERNEL_HANDLE,
                                                    KernelMode );

                ntStatus = LdrpLoadUserModuleEx( &SectionHandle,
                                                 FileName.Buffer );

                if ( !NT_SUCCESS( ntStatus ) ) {

                    goto LdrpImportFinished;
                }

                BaseAddress = NULL;
                ntStatus = ZwMapViewOfSection( SectionHandle,
                                               ProcessHandle,
                                               &BaseAddress,
                                               0,
                                               0,
                                               PAGE_READ | PAGE_WRITE | PAGE_EXECUTE );

                ntStatus = LdrResolveImportTable( LoadBase, BaseAddress, Import );

                ZwClose( SectionHandle );
                ZwClose( ProcessHandle );

                Import++;
                continue;
            }
        }
    LdrpImportFinished:;
    }

    return ntStatus;
}
