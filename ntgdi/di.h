

#pragma once

#define NtGdiAllocateDi( type ) ExAllocatePoolWithTag( sizeof( type ), TAGEX_DI )

VOID
NtGdiDiInsert(
	__in PKWINDOW Window,
	__in PDI_BASE DiToInsert
);

//
//	diwinbase.c
//

typedef struct _DI_WINDOW_BASE {
	DI_BASE Base;

	ULONG32 Fill;
	ULONG32 Border;
	ULONG32 Flags;

} DI_WINDOW_BASE, *PDI_WINDOW_BASE;

VOID
NtGdiDiWindowBaseClassInit(
	__in PKWINDOW Window,
	__in PWNDCLASSEX Class
);

VOID
NtGdiDiWindowBaseDrawProc(
	__in PKWINDOW Window,
	__in PDI_WINDOW_BASE WindowBase
);

//
//	diwinbase.c
//


//
//	dibutton.c
//

typedef struct _DI_BUTTON {
	DI_BASE Base;

	ULONG32 Fill;
	ULONG32 Border;

} DI_BUTTON, *PDI_BUTTON;

VOID
NtGdiDiButtonWndProc(
	__in PKWINDOW Window,
	__in ULONG32 Message,
	__in ULONG32 Param1,
	__in ULONG32 Param2
);

VOID
NtGdiDiButtonClassInit(
	__in PKWINDOW Window,
	__in PWNDCLASSEX Class
);

VOID
NtGdiDiButtonDrawProc(
	__in PKWINDOW Window,
	__in PDI_BUTTON Button
);

//
//	dibutton.c
//


//
//    diconbase.c
//

typedef struct _DI_CONSOLE_BASE {
	DI_BASE Base;

	ULONG32 Fill;
	ULONG32 Border;


} DI_CONSOLE_BASE, *PDI_CONSOLE_BASE;

VOID
NtGdiDiConsoleBaseWndProc(
	__in PKWINDOW Window,
	__in ULONG32 Message,
	__in ULONG32 Param1,
	__in ULONG32 Param2
);

VOID
NtGdiDiConsoleBaseClassInit(
	__in PKWINDOW Window,
	__in PWNDCLASSEX Class
);

VOID
NtGdiDiConsoleBaseDrawProc(
	__in PKWINDOW Window,
	__in PDI_CONSOLE_BASE ConsoleBase
);

//
//    diconbase.c
//


//
//	dieditbox.c
//

typedef struct _CS_EDIT_BOX {
	int pd0;

} CS_EDIT_BOX, *PCS_EDIT_BOX;

typedef struct _DI_EDIT_BOX {
	DI_BASE Base;

	ULONG32 Fill;
	ULONG32 Border;


} DI_EDIT_BOX, *PDI_EDIT_BOX;

VOID
NtGdiDiEditboxWndProc(
	__in PKWINDOW Window,
	__in ULONG32 Message,
	__in ULONG32 Param1,
	__in ULONG32 Param2
);

VOID
NtGdiDiEditboxClassInit(
	__in PKWINDOW Window,
	__in PWNDCLASSEX Class
);

VOID
NtGdiDiEditboxDrawProc(
	__in PKWINDOW Window,
	__in PDI_EDIT_BOX Editbox
);

//
//	dieditbox.c
//


