


#include <carbsup.h>
#include "ldrp.h"

NTSTATUS
LdrGetExportAddressByName(
    _In_  PVOID  Base,
    _In_  PCHAR  Name,
    _Out_ PVOID* Address
)
{
    PIMAGE_DOS_HEADER Dos;
    PIMAGE_NT_HEADERS Nt;
    PIMAGE_EXPORT_DIRECTORY Export;
    PULONG FunctionTable;
    PULONG NameTable;
    PUSHORT OrdinalTable;
    ULONG CurrentOrdinal;
    PCHAR CurrentName;

    Dos = ( PIMAGE_DOS_HEADER )Base;
    
    if ( !LdrpCheckDos( Dos ) ) {

        return STATUS_INVALID_IMAGE;
    }

    Nt = ( PIMAGE_NT_HEADERS )( ( PUCHAR )Base + Dos->e_lfanew );

    if ( !LdrpCheckNt( Nt ) ) {

        return STATUS_INVALID_IMAGE;
    }

    if ( Nt->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXPORT ].Size == 0 ) {

        return STATUS_INVALID_IMAGE;
    }

    Export = ( PIMAGE_EXPORT_DIRECTORY )( ( PUCHAR )Base + Nt->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXPORT ].VirtualAddress );
    FunctionTable = ( PULONG )( ( PUCHAR )Base + Export->AddressOfFunctions );
    NameTable = ( PULONG )( ( PUCHAR )Base + Export->AddressOfNames );
    OrdinalTable = ( PUSHORT )( ( PUCHAR )Base + Export->AddressOfNameOrdinals );

    for ( CurrentOrdinal = 0; CurrentOrdinal < Export->NumberOfNames; CurrentOrdinal++ ) {
        CurrentName = ( ( PCHAR )Base + NameTable[ CurrentOrdinal ] );

        if ( RtlAnsiStringCompare( Name, CurrentName ) == 0 ) {
            *Address = ( PVOID )( ( PUCHAR )Base + FunctionTable[ OrdinalTable[ CurrentOrdinal ] ] );
            return STATUS_SUCCESS;
        }
    }

    return STATUS_NOT_FOUND;
}

NTSTATUS
LdrGetExportAddressByOrdinal(
    _In_  PVOID  Base,
    _In_  USHORT Ordinal,
    _Out_ PVOID* Address
)
{
    PIMAGE_DOS_HEADER Dos;
    PIMAGE_NT_HEADERS Nt;
    PIMAGE_EXPORT_DIRECTORY Export;
    PULONG FunctionTable;
    PUSHORT OrdinalTable;
    ULONG CurrentOrdinal;
    USHORT OrdinalValue;

    Dos = ( PIMAGE_DOS_HEADER )Base;

    if ( !LdrpCheckDos( Dos ) ) {

        return STATUS_INVALID_IMAGE;
    }

    Nt = ( PIMAGE_NT_HEADERS )( ( PUCHAR )Base + Dos->e_lfanew );

    if ( !LdrpCheckNt( Nt ) ) {

        return STATUS_INVALID_IMAGE;
    }

    if ( Nt->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXPORT ].Size == 0 ) {

        return STATUS_NOT_FOUND;
    }

    Export = ( PIMAGE_EXPORT_DIRECTORY )( ( PUCHAR )Base + Nt->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXPORT ].VirtualAddress );
    FunctionTable = ( PULONG )( ( PUCHAR )Base + Export->AddressOfFunctions );
    OrdinalTable = ( PUSHORT )( ( PUCHAR )Base + Export->AddressOfNameOrdinals );

    for ( CurrentOrdinal = 0; CurrentOrdinal < Export->NumberOfFunctions; CurrentOrdinal++ ) {
        OrdinalValue = *( PUSHORT )( ( PUCHAR )Base + OrdinalTable[ CurrentOrdinal ] );

        if ( OrdinalValue == Ordinal ) {
            *Address = ( PVOID )( ( PUCHAR )Base + FunctionTable[ OrdinalTable[ CurrentOrdinal ] ] );
            return STATUS_SUCCESS;
        }
    }

    return STATUS_NOT_FOUND;
}

NTSTATUS
LdrResolveBaseReloc(
    _In_ PVOID Base
)
{
    PIMAGE_DOS_HEADER Dos;
    PIMAGE_NT_HEADERS Nt;
    PIMAGE_BASE_RELOCATION BaseReloc;
    LONG64 BaseDelta;
    ULONG64 RelocCount;
    PUSHORT RelocList;
    ULONG64 CurrentReloc;
    PLONG64 Address;

    Dos = ( PIMAGE_DOS_HEADER )Base;

    if ( !LdrpCheckDos( Dos ) ) {

        return STATUS_INVALID_IMAGE;
    }

    Nt = ( PIMAGE_NT_HEADERS )( ( PUCHAR )Base + Dos->e_lfanew );

    if ( !LdrpCheckNt( Nt ) ) {

        return STATUS_INVALID_IMAGE;
    }

    if ( Nt->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_BASERELOC ].Size == 0 ||
        ( Nt->OptionalHeader.DllCharacteristics & IMAGE_FILE_RELOCS_STRIPPED ) != 0 ) {

        return STATUS_INVALID_IMAGE;
    }

    BaseReloc = ( PIMAGE_BASE_RELOCATION )( ( PUCHAR )Base + Nt->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_BASERELOC ].VirtualAddress );
    BaseDelta = ( LONG64 )Base - ( LONG64 )Nt->OptionalHeader.ImageBase;

    if ( BaseDelta == 0 ) {

        return STATUS_SUCCESS;
    }

    while ( BaseReloc->VirtualAddress ) {

        if ( BaseReloc->SizeOfBlock > sizeof( IMAGE_BASE_RELOCATION ) ) {
            RelocCount = ( BaseReloc->SizeOfBlock - sizeof( IMAGE_BASE_RELOCATION ) ) / sizeof( USHORT );
            RelocList = ( PUSHORT )( ( PUCHAR )BaseReloc + sizeof( IMAGE_BASE_RELOCATION ) );

            for ( CurrentReloc = 0; CurrentReloc < RelocCount; CurrentReloc++ ) {

                if ( RelocList[ CurrentReloc ] != 0 ) {
                    Address = ( PLONG64 )( ( PUCHAR )Base + BaseReloc->VirtualAddress + ( RelocList[ CurrentReloc ] & 0xfff ) );
                    *Address += BaseDelta;
                }
            }
        }

        BaseReloc = ( PIMAGE_BASE_RELOCATION )( ( PUCHAR )BaseReloc + BaseReloc->SizeOfBlock );
    }

    return STATUS_SUCCESS;
}

NTSTATUS
LdrResolveImportTable(
    _In_ PVOID                    Importer,
    _In_ PVOID                    Importee,
    _In_ PIMAGE_IMPORT_DESCRIPTOR Import
)
{
    NTSTATUS ntStatus;
    PIMAGE_THUNK_DATA OriginalFirstThunk;
    PIMAGE_THUNK_DATA FirstThunk;
    PIMAGE_IMPORT_BY_NAME Name;

    OriginalFirstThunk = ( PIMAGE_THUNK_DATA )( ( PUCHAR )Importer + Import->OriginalFirstThunk );
    FirstThunk = ( PIMAGE_THUNK_DATA )( ( PUCHAR )Importer + Import->FirstThunk );

    while ( OriginalFirstThunk->u1.AddressOfData ) {

        if ( OriginalFirstThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG ) {
            ntStatus = LdrGetExportAddressByOrdinal(
                Importee,
                ( USHORT )( OriginalFirstThunk->u1.Ordinal & 0xFFFF ),
                ( PVOID* )&FirstThunk->u1.Function );
            if ( !NT_SUCCESS( ntStatus ) ) {

                return ntStatus;
            }
        }
        else {
            Name = ( PIMAGE_IMPORT_BY_NAME )( ( PUCHAR )Importer + OriginalFirstThunk->u1.AddressOfData );
            ntStatus = LdrGetExportAddressByName(
                Importee,
                ( PCHAR )Name->Name,
                ( PVOID* )&FirstThunk->u1.Function );
            if ( !NT_SUCCESS( ntStatus ) ) {

                return ntStatus;
            }
        }

        OriginalFirstThunk++;
        FirstThunk++;
    }

    return STATUS_SUCCESS;
}
