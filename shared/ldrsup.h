


#pragma once

typedef struct _LDR_INFO_BLOCK {
	PVOID ModuleStart;
	PVOID ModuleEnd;
	PVOID ModuleEntry;
} LDR_INFO_BLOCK, *PLDR_INFO_BLOCK;

typedef struct _KMODULE {
	/*
		structure for a ModuleObject,
	*/
	UNICODE_STRING ImageName;
	LDR_INFO_BLOCK LoaderInfoBlock;
} KMODULE, *PKMODULE;

#define REASON_DLL_LOAD   (0x00000001L)
#define REASON_DLL_UNLOAD (0x00000002L)

typedef VOID( *KDLL_ENTRY )(
	__in ULONG64 Base,
	__in ULONG32 Reason
	);

NTSTATUS
LdrLoadDll(
	__in PUNICODE_STRING FilePath
);

typedef struct _VAD {
	LDR_INFO_BLOCK Range;
	UNICODE_STRING RangeName;

	struct _VAD*   Next;
} VAD, *PVAD;