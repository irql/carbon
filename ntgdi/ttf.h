


#pragma once

//https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6.html

typedef struct _TTF_HEADER {

	ULONG32 Version;
	ULONG32 TableCount;
	ULONG32 SearchRange;
	ULONG32 EntrySelector;
	ULONG32 RangeShift;

} TTF_HEADER, *PTTF_HEADER;

typedef struct _TTF_TABLE_ENTRY {

	ULONG32 Tag;
	ULONG32 Checksum;
	ULONG32 Offset;
	ULONG32 Length;

} TTF_TABLE_ENTRY, *PTTF_TABLE_ENTRY;

#define TTF_HEAD_SIG 'daeh' 

#define TTF_HEAD_TABLE_MAGIC 0x5F0F3CF5

typedef struct _TTF_HEAD_TABLE {

	ULONG32 TableVersion;
	ULONG32 FontRevision;
	ULONG32 ChecksumAdjustment;
	ULONG32 MagicNumber;
	USHORT  Flags; //define flags.
	USHORT  UnitsPerEm;
	LONG64  CreatedTime;
	LONG64  ModifiedTime;
	SHORT   xMin;
	SHORT   yMin;
	SHORT   xMax;
	SHORT   yMax;
	USHORT  MacStyle;
	USHORT  SmallestSizeInPx;
	SHORT   FontDirectionHint;
	SHORT   IndexToLocFormat; // 0 SHORT OFFSETS, 1 LONG OFFSETS
	SHORT   GlyphDataFormat;

} TTF_HEAD_TABLE, *PTTF_HEAD_TABLE;

#define TTF_MAX_PROFILE_SIG 'pxam'

typedef struct _TTF_MAX_PROFILE_TABLE {

	ULONG32 Version;
	USHORT  GlyphCount;
	USHORT  MaxPoints;
	USHORT  MaxContours;
	USHORT  MaxComponentPoint;
	USHORT  MaxComponentContours;
	USHORT  MaxZones;
	USHORT  MaxTwilightPoints;
	USHORT  MaxStorage;
	USHORT  MaxFunctionDefs;
	USHORT  MaxInstructionDefs;
	USHORT  MaxStackElements;
	USHORT  MaxSizeOfInstructions;
	USHORT  MaxComponentElements;
	USHORT  MaxComponentDepth;

} TTF_MAX_PROFILE_TABLE, *PTTF_MAX_PROFILE_TABLE;

#define TTF_NAME_SIG 'eman'

typedef struct _TTF_NAME_RECORD {
	USHORT PlatformId;
	USHORT PlatformSpecificId;
	USHORT LanguageId;
	USHORT NameId;
	USHORT Length;
	USHORT Offset;

} TTF_NAME_RECORD, *PTTF_NAME_RECORD;

typedef struct _TTF_NAME_TABLE {
	USHORT Format;
	USHORT Count;
	USHORT StringOffset;
	TTF_NAME_RECORD NameRecords[ 0 ];

} TTF_NAME_TABLE, *PTTF_NAME_TABLE;

#define TTF_LOCA_SIG 'acol'

typedef struct _TTF_SHORT_LOCA_TABLE {

	USHORT Offsets[ 0 ];

} TTF_SHORT_LOCA_TABLE, *PTTF_SHORT_LOCA_TABLE;

typedef struct _TTF_LONG_LOCA_TABLE {

	ULONG32 Offsets[ 0 ];

} TTF_LONG_LOCA_TABLE, *PTTF_LONG_LOCA_TABLE;

#define TTF_GLYF_SIG 'fylg'

typedef struct _TTF_GLYF_TABLE {

	SHORT ContoursCount;
	SHORT xMin;
	SHORT yMin;
	SHORT xMax;
	SHORT yMax;

	//(here follow the data for the simple or compound glyph)
} TTF_GLYF_TABLE, *PTTF_GLYF_TABLE;

#define TTF_CMAP_SIG 'pamc'

typedef struct _TTF_CMAP_TABLE {
	USHORT Version;
	USHORT SubtablesCount;

} TTF_CMAP_TABLE, *PTTF_CMAP_TABLE;

typedef struct _TTF_CMAP_SUBTABLES {
	USHORT  PlatformId;
	USHORT  PlatformSpecificId;
	ULONG32 Offset;

} TTF_CMAP_SUBTABLES, *PTTF_CMAP_SUBTABLES;

