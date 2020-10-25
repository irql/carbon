


#pragma once

typedef struct _KWINDOW *PKWINDOW;
typedef struct _WNDCLASSEX *PWNDCLASSEX;

typedef VOID( *WNDPROC )(
	__in PKWINDOW,
	__in ULONG32,
	__in ULONG32,
	__in ULONG32
	);

typedef VOID( *KWNDCLASSINIT )(
	__in PKWINDOW,
	__in PWNDCLASSEX
	);

typedef struct _WNDCLASSEX {

	WNDPROC WndProc;
	UNICODE_STRING ClassName;

	ULONG32 Flags;

	ULONG32 Border;
	ULONG32 Fill;

	KWNDCLASSINIT ClassInit;

} WNDCLASSEX, *PWNDCLASSEX;

typedef struct _WNDCLASSEX_INTERNAL {

	WNDPROC WndProc;
	UNICODE_STRING ClassName;

	ULONG32 Flags;

	ULONG32 Border;
	ULONG32 Fill;

	KWNDCLASSINIT ClassInit;

	LIST_ENTRY ClassLinks;

} WNDCLASSEX_INTERNAL, *PWNDCLASSEX_INTERNAL;

typedef struct _DI_BASE *PDI_BASE;

typedef struct _KWINDOW {
	KSPIN_LOCK CompositeLock;

	RECT Rect;
	ULONG32 Flags;
	ULONG32 MenuId;

	UNICODE_STRING Name;

	ULONG32* PrimaryBuffer;

	PDI_BASE DiFlink;

	PWNDCLASSEX Class;

	struct _KWINDOW* Next; //only valid if it's not a child.
	struct _KWINDOW* Child; //can be null.
	struct _KWINDOW* Parent;

} KWINDOW, *PKWINDOW;

typedef VOID( *DI_DRAW_PROCEDURE )(
	__in PKWINDOW,
	__in PDI_BASE
	);

typedef struct _DI_BASE {

	PDI_BASE DiFlink;
	DI_DRAW_PROCEDURE DrawProcedure;

} DI_BASE, *PDI_BASE;

#include "di.h"

EXTERN KSPIN_LOCK g_WindowListLock;

EXTERN KLOCKED_LIST g_ClassList;
EXTERN POBJECT_TYPE_DESCRIPTOR ObjectTypeWindow;

VOID
NtGdiWndProcDefault(
	__in PKWINDOW Window,
	__in ULONG32 Message,
	__in ULONG32 Param1,
	__in ULONG32 Param2
);

NTSTATUS
NtGdiWindowsInitializeSubsystem(

);

VOID
NtGdiWindowComposite(
	__in PKWINDOW WindowObject
);

NTSTATUS
NtGdiCreateWindow(
	__in PUNICODE_STRING Name,
	__in PUNICODE_STRING ClassName,
	__in ULONG32 Flags,
	__in ULONG32 x,
	__in ULONG32 y,
	__in ULONG32 Width,
	__in ULONG32 Height,
	__in ULONG32 MenuId,
	__in HANDLE ParentHandle,
	__out PHANDLE Handle
);

NTSTATUS
NtGdiRegisterClass(
	__in PWNDCLASSEX Class
);

NTSTATUS
NtGdiUnregisterClass(
	__in PWNDCLASSEX Class
);

PWNDCLASSEX
NtGdiFindClassByName(
	__in PUNICODE_STRING ClassName
);

#define WND_TITLE_BAR_H     32
#define WND_TITLE_INDENT_X  12
#define WND_TITLE_INDENT_Y  10

EXTERN PKWINDOW g_Focus;
EXTERN PKWINDOW g_RootWindow;

#define WM_KEY_PRESS                (0x00000001L)
#define WM_MOUSE_CLICK              (0x00000002L)
#define WM_FOCUS                    (0x00000003L)
#define WM_COMMAND                  (0x00000004L)

PKWINDOW
NtGdiWindowFromPoint(
	__in PPOINT Point
);

VOID
NtGdiWindowInsert(
	__in PKWINDOW Insert
);

VOID
NtGdiWindowRemove(
	__in PKWINDOW Remove
);
