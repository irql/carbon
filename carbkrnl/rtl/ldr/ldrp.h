


#pragma once

FORCEINLINE
BOOLEAN
LdrpCheckDos(
    _In_ PIMAGE_DOS_HEADER Dos
)
{
    return Dos->e_magic == IMAGE_DOS_SIGNATURE;
}

FORCEINLINE
BOOLEAN
LdrpCheckNt(
    _In_ PIMAGE_NT_HEADERS Nt
)
{
    return Nt->Signature == IMAGE_NT_SIGNATURE;
}

FORCEINLINE
BOOLEAN
LdrpIsRelocImage(
    _In_ PIMAGE_NT_HEADERS Nt
)
{
    return ( Nt->OptionalHeader.DllCharacteristics & IMAGE_FILE_RELOCS_STRIPPED ) != IMAGE_FILE_RELOCS_STRIPPED;
}

NTSTATUS
LdrGetExportAddressByName(
    _In_  PVOID  Base,
    _In_  PCHAR  Name,
    _Out_ PVOID* Address
);

NTSTATUS
LdrGetExportAddressByOrdinal(
    _In_  PVOID  Base,
    _In_  USHORT Ordinal,
    _Out_ PVOID* Address
);

NTSTATUS
LdrResolveBaseReloc(
    _In_ PVOID Base
);

NTSTATUS
LdrResolveImportTable(
    _In_ PVOID                    Importer,
    _In_ PVOID                    Importee,
    _In_ PIMAGE_IMPORT_DESCRIPTOR Import
);

NTSTATUS
LdrpGetLoaderLimits(
    _In_    PVOID    FileLoaded,
    _Inout_ PULONG64 ModuleLength
);

NTSTATUS
LdrpLoadSupervisorModule(
    _In_ PKPROCESS Process,
    _In_ PMM_VAD   Vad,
    _In_ PVOID     LoadBase,
    _In_ ULONG64   LoadLength,
    _In_ PVOID     FileBase,
    _In_ ULONG64   FileLength
);

NTSTATUS
LdrpLoadUserModule(
    _In_ PKPROCESS Process,
    _In_ PMM_VAD   Vad,
    _In_ PVOID     LoadBase,
    _In_ ULONG64   LoadLength,
    _In_ PVOID     FileBase,
    _In_ ULONG64   FileLength
);

VOID
LdrpMapUserSection(
    _In_ PKPROCESS Process,
    _In_ PMM_VAD   Vad
);
