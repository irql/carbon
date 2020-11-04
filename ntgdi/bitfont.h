


#pragma once

VOID
NtGdiBitFontRender(
	__in PKFONT Font,
	__in PWCHAR Text,
	__in ULONG32 x,
	__in ULONG32 y,
	__in ULONG32 Colour,
	__in ULONG32* Buffer,
	__in PRECT Region
);

VOID
NtGdiBitFontExtentPoint(
	__in PKFONT Font,
	__in PWCHAR Buffer,
	__in PPOINT Region,
	__in PPOINT ExtentPoint
);

VOID
NtGdiBitFontEndPoint(
	__in PKFONT Font,
	__in PWCHAR Buffer,
	__in PPOINT Region,
	__in PPOINT EndPoint
);