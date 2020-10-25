

#include "fat32.h"

FAT32_PATH_TYPE
FsFat32VerifyFileName(
	__in PWCHAR FileName
	)
{
	FAT32_PATH_TYPE PathType = Fat32Path8Dot3;

	ULONG32 Length = _wcslen(FileName);
	ULONG32 DotPos = Length + 1;

	for (ULONG32 i = 0; i < Length; i++) {

		//
		//	0x05 to be replaced with 0xE5.
		//

		if (FileName[i] != 0x05 && 
			FileName[i] < 0x20)
			return Fat32PathInvalid;

		if (PathType == Fat32Path8Dot3 && FileName[i] > 0x7E)
			PathType = Fat32PathLongFileName;
		
		//
		//	only ASCII and UCS-2, this filters out
		//	any UTF-16 surrogate pairs.
		//	they are in the range of 0xD800 - 0xDFFF (first wchar)
		//

		if (FileName[i] >= 0xD800 &&
			FileName[i] <= 0xDF00)
			return Fat32PathInvalid;

		if (FileName[i] == '.')
			DotPos = i;

	}
	
	if (PathType == Fat32Path8Dot3) {
		if (DotPos != (Length + 1)) {

			if (DotPos > 8)
				PathType = Fat32PathLongFileName;

			if (Length > 12)
				PathType = Fat32PathLongFileName;

		}
		else {
			
			//PathType = Fat32PathLongFileName;

			//8.3 must have a dot. No.
		}

		if (Length > 11)
			PathType = Fat32PathLongFileName;

	}

	return PathType;
}

/*
	removed any verification, that should be done in the above.
*/
VOID
FsFat32ConvertPathTo8Dot3(
	__in PWCHAR FileName,
	__out PCHAR FileName8Dot3
	)
{
	ULONG32 FileNameLength = _wcslen(FileName);
	ULONG32 DotPosition = FileNameLength + 1;

	for (ULONG32 i = FileNameLength; i > 0; i--) {
		if (FileName[i] == '.') {
			DotPosition = i;
			break;
		}
	}

	_memset(FileName8Dot3, ' ', 11);
	FileName8Dot3[11] = 0;

	for (ULONG32 i = 0; i < DotPosition && FileName[i]; i++)
		FileName8Dot3[i] = (CHAR)UPPER(FileName[i]);

	if (DotPosition < FileNameLength) {

		DotPosition++;

		for (ULONG32 i = DotPosition; (i - DotPosition) < 3; i++)
			FileName8Dot3[(DotPosition <= 8 ? 8 : DotPosition) + (i - DotPosition)] = (CHAR)UPPER(FileName[i]);
	}

	return;
}

UCHAR
FsFat32NameChecksum(
	__in PCHAR ShortName
	)
{

	UCHAR Checksum = 0;

	for (UCHAR Length = 11; Length != 0; Length--) {

		//should gen ror (rot right)
		Checksum = ((Checksum & 1) ? 0x80 : 0) + (Checksum >> 1) + *ShortName++;
	}

	return Checksum;
}

ULONG64
FsFat32CompareLfnEntry(
	__in PFAT32_LFN_DIRECTORY_ENTRY LfnDirectory,
	__in PWCHAR Lfn13Chars
	)
{
	ULONG32 Length = _wcslen(Lfn13Chars);
	ULONG32 Return;

	if (Length > 13)
		Length = 13;

	if (Length <= 5) {

		return wcsncmp(LfnDirectory->First5Chars, Lfn13Chars, Length);
	}

	Return = wcsncmp(LfnDirectory->First5Chars, Lfn13Chars, 5);
	if (Return != 0)
		return Return;

	Length -= 5;

	if (Length <= 6) {

		return wcsncmp(LfnDirectory->Next6Chars, Lfn13Chars + 5, Length);
	}

	Return = wcsncmp(LfnDirectory->Next6Chars, Lfn13Chars + 5, 6);
	if (Return != 0)
		return Return;

	Length -= 6;

	for (ULONG32 i = 0; i < Length; i++) {
		
		Return = LfnDirectory->Next2Chars[i] - Lfn13Chars[11 + i];

		if (Return != 0)
			return Return;
	}

	/*
	if (LfnDirectory->Next2Chars[0] != Lfn13Chars[11] ||
		LfnDirectory->Next2Chars[1] != Lfn13Chars[12])
		return 1;*/

	return 0;
}

ULONG64
FsFat32FindFile(
	__in PFAT32_DIRECTORY_ENTRY Directory,
	__in FAT32_PATH_TYPE PathType,
	__in PWCHAR FileName
	)
{

	if (PathType == Fat32Path8Dot3) {
		PCHAR Fat32FileName = (PCHAR)FileName;

		for (ULONG64 i = 0; Directory[i].Name[0] != 0; i++) {

			if ((UCHAR)Directory[i].Name[0] == (UCHAR)FAT32_DIRECTORY_ENTRY_FREE)
				continue;

			if ((UCHAR)Directory[i].Attributes & FAT32_VOLUME_ID)
				continue;

			if (strncmp(Directory[i].Name, Fat32FileName, 11) == 0) {

				return i;
			}
		}
	}
	else if (PathType == Fat32PathLongFileName) {
		PFAT32_LFN_DIRECTORY_ENTRY LfnDirectory = (PFAT32_LFN_DIRECTORY_ENTRY)Directory;
		ULONG64 LongNameFound = (ULONG64)-1, ShortNameFound = (ULONG64)-1;

		for (ULONG64 i = 0; LfnDirectory[i].OrderOfEntry != 0; i++) {

			if (LfnDirectory[i].Attributes != FAT32_LFN)
				continue;

			if ((LfnDirectory[i].OrderOfEntry & ~(UCHAR)FAT32_LAST_LFN_ENTRY) != 0x1)
				continue;

			ULONG64 j = i;
			ULONG32 FileNameIndex = 0;
			do {

				if (FsFat32CompareLfnEntry(&LfnDirectory[i], (PWCHAR)(FileName + FileNameIndex)) != 0)
					break;

				if (LfnDirectory[i].OrderOfEntry & FAT32_LAST_LFN_ENTRY) {
					LongNameFound = i;
					ShortNameFound = j + 1;
					break;
				}
				i--;

				FileNameIndex += 13;
			} while (1);

			if (LongNameFound != (ULONG64)-1)
				break;

			i = j;
		}

		if (LongNameFound == (ULONG64)-1)
			return LongNameFound;

		if (FsFat32NameChecksum((PCHAR)&Directory[ShortNameFound].Name) == LfnDirectory[LongNameFound].NameChecksum)
			return ShortNameFound;

		//Fucking Orphan.
		return (ULONG64)-1;
	}

	return (ULONG64)-1;
}
