


#pragma once

typedef struct _KFONT *PKFONT;

typedef VOID( *KFONT_RENDER )(
	__in PKFONT Font,
	__in PWCHAR Text,
	__in ULONG32 x,
	__in ULONG32 y,
	__in ULONG32 Colour,
	__in ULONG32* Buffer,
	__in PRECT Region
	);

typedef VOID( *KFONT_EXTENTPOINT )(
	__in PKFONT Font,
	__in PWCHAR Buffer,
	__in PPOINT Region,
	__in PPOINT ExtentPoint
	);

typedef VOID( *KFONT_ENDPOINT )(
	__in PKFONT Font,
	__in PWCHAR Buffer,
	__in PPOINT Region,
	__in PPOINT EndPoint
	);

typedef struct _KFONT {
	PVOID FileBuffer;
	ULONG64 FileSize;

	ULONG32 FontType; //indicates if its a bmp, ttf, otf, etc

	KFONT_RENDER Render;
	KFONT_EXTENTPOINT ExtentPoint;
	KFONT_ENDPOINT EndPoint;


} KFONT, *PKFONT;

NTSTATUS
NtGdiInitializeBitFont(
	__in PKFONT Font
);

NTSTATUS
NtGdiFontInitializeObject(

);

NTSTATUS
NtGdiInitializeFont(
	__in PKFONT FontObject
);

NTSTATUS
NtGdiCreateFont(
	__out PHANDLE FontHandle,
	__in PUNICODE_STRING FontFile
);

NTSTATUS
NtGdiFontExtentPoint(
	__in HANDLE FontHandle,
	__in PWCHAR Buffer,
	__in PPOINT Region,
	__in PPOINT ExtentPoint
);

NTSTATUS
NtGdiFontEndPoint(
	__in HANDLE FontHandle,
	__in PWCHAR Buffer,
	__in PPOINT Region,
	__in PPOINT EndPoint
);

EXTERN PKFONT g_DefaultFont;