


#pragma once

#define BITMAP_MAGIC 'MB'

typedef struct _BITMAP_HEADER {
	USHORT Magic;
	ULONG32 FileSize;
	ULONG32 Reserved;
	ULONG32 ImageOffset;
	ULONG32 DibHeaderSize;
	ULONG32 Width;
	ULONG32 Height;
	USHORT NumberOfPlanes;
	USHORT BitsPerPixel;
	ULONG32 Compression;
	ULONG32 ImageSize;
	ULONG32 XResolutionPpm;
	ULONG32 YResolutionPpm;
	ULONG32 NumberOfColours;
	ULONG32 ImportantColours;
} BITMAP_HEADER, *PBITMAP_HEADER;
