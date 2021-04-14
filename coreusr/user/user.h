


#pragma once

NTSTATUS
NtClassEditBaseProc(
    _In_ HANDLE  WindowHandle,
    _In_ ULONG32 MessageId,
    _In_ ULONG64 Param1,
    _In_ ULONG64 Param2
);

NTSTATUS
NtClassStaticBaseProc(
    _In_ HANDLE  WindowHandle,
    _In_ ULONG32 MessageId,
    _In_ ULONG64 Param1,
    _In_ ULONG64 Param2
);

NTSTATUS
NtClassButtonBaseProc(
    _In_ HANDLE  WindowHandle,
    _In_ ULONG32 MessageId,
    _In_ ULONG64 Param1,
    _In_ ULONG64 Param2
);

NTSTATUS
NtClassListViewBaseProc(
    _In_ HANDLE  WindowHandle,
    _In_ ULONG32 MessageId,
    _In_ ULONG64 Param1,
    _In_ ULONG64 Param2
);

typedef struct _MENU_ITEM {
    PWSTR              Name;
    BOOLEAN            Open;
    ULONG32            MenuId;
    struct _MENU_ITEM* Link;
    struct _MENU_ITEM* Child;
} MENU_ITEM, *PMENU_ITEM;

typedef struct _NT_MENU_HANDLE {
    PMENU_ITEM MenuItem;
} NT_MENU_HANDLE, *PNT_MENU_HANDLE;

//make width shit more dynamic and not hardcoded to 36.
#define WB_MENU_ITEM_WIDTH  36
#define WB_MENU_ITEM_HEIGHT 16
