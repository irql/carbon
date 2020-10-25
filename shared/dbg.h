


#pragma once

typedef struct _DBG_LINE {
	LIST_ENTRY LineLinks;
	ULONG32 LineNumber;
	WCHAR LineBuffer[256 - ((sizeof(ULONG32) + sizeof(LIST_ENTRY)) / sizeof(WCHAR))];
} DBG_LINE, *PDBG_LINE;

NTSYSAPI PLIST_ENTRY DbgLineHeader;

NTSYSAPI
VOID
DbgPrint(
	__in PWSTR Format,
	__in ...
	);

