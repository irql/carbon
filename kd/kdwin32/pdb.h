


#pragma once

//
// Couldn't be bothered finding the code view 4.0 
// specifications so I just gathered this from
// opening a PE format image in CFF explorer
// and viewing the debug directory entry.
//

#pragma pack(push, 1)
typedef struct _CODE_VIEW_ENTRY {
    ULONG32 Signature; // RSDS, 'SDSR'
    ULONG32 Unk[ 5 ];
    CHAR    PdbFile[ 0 ];

} CODE_VIEW_ENTRY, *PCODE_VIEW_ENTRY;
#pragma pack(pop)

#ifdef __dia2_h__

typedef struct _KPDB_CONTEXT {
    WCHAR           FileName[ 256 ];
    PVOID           Cached;
    IDiaDataSource* Source;
    IDiaSession*    Session;
    IDiaSymbol*     Global;

} KPDB_CONTEXT, *PKPDB_CONTEXT;

#else

typedef long HRESULT;
typedef struct _KPDB_CONTEXT *PKPDB_CONTEXT;

#endif

typedef struct _KD_CACHED_MODULE {
    WCHAR         Name[ 256 ];
    LIST_ENTRY    CacheLinks;
    PKPDB_CONTEXT PdbContext;
    BOOLEAN       HasPdb;
    BOOLEAN       KernelSpace;

    //
    // ProcessCache is the last process which read
    // the vad for it, base addresses can change from 
    // process to process but the data itself never does.
    //

    PVOID        ProcessCache;
    ULONG64      Start;
    ULONG64      End;

} KD_CACHED_MODULE, *PKD_CACHED_MODULE;

extern PKD_CACHED_MODULE KdpKernelContext;

HRESULT
DbgInitialize(

);

HRESULT
DbgLoadPdb(
    _In_  PKD_CACHED_MODULE Module,
    _In_  PWSTR             PdbFileName,
    _Out_ PKPDB_CONTEXT     Context
);

HRESULT
DbgFieldOffset(
    _In_  PKD_CACHED_MODULE Context,
    _In_  PCWSTR            TypeName,
    _In_  PCWSTR            FieldName,
    _Out_ LONG*             FieldOffset
);

extern "C"
HRESULT
DbgGetFunctionByAddress(
    _In_  PKD_CACHED_MODULE Context,
    _In_  ULONG64           Address,
    _Out_ PWCHAR*           FunctionName,
    _Out_ PWCHAR*           FileName,
    _Out_ PULONG32          LineNumber,
    _Out_ PLONG             FunctionDisplacement
);

extern "C"
PKPDB_CONTEXT
DbgCreateDebugContext(
    _In_ PWSTR             FileName,
    _In_ PKD_CACHED_MODULE Cached
);

HRESULT
DbgGetAddressByName(
    _In_  PKD_CACHED_MODULE Context,
    _In_  PCWSTR            Name,
    _Out_ ULONG*            Rva
);

HRESULT
DbgGetFunctionByName(
    _In_  PKD_CACHED_MODULE Context,
    _In_  PCWSTR            Name,
    _Out_ ULONG*            Rva
);

PKD_CACHED_MODULE
KdpGetModuleByAddress(
    _In_ ULONG64 Address
);

PKD_CACHED_MODULE
KdpCacheModule(
    _In_ PMM_VAD Vad
);

extern "C"
PKD_CACHED_MODULE
KdpGetModuleByShort(
    _In_ PWCHAR ShortName
);

extern "C"
HRESULT
DbgPrintFunctionFrame(
    _In_ PKD_CACHED_MODULE Context,
    _In_ PWCHAR            FunctionName,
    _In_ ULONG64           FrameBase
);
