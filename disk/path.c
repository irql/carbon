


#include "driver.h"

STATIC WCHAR FspIllegalCharacters[] = {
	'/', ':', '*', '?', '\"', '<', '>', '|'
};

NTSTATUS
FsSplitDirectoryPath(
	__in PWCHAR DirectoryPath,
	__out PWCHAR** SplitPath
	)
{
	ULONG32 PathLength = _wcslen(DirectoryPath);
	ULONG32 BufferSize = 8;
	
	if (DirectoryPath[0] != '\\')
		return STATUS_INVALID_PATH;

	if (DirectoryPath[1] == 0) {
		/*
			root directory, aka \
		*/

		*SplitPath = (PWCHAR*)ExAllocatePoolWithTag(BufferSize, 'htaP');
		(*SplitPath)[0] = NULL;

		return STATUS_SUCCESS;
	}

	for (ULONG32 i = 0; i < PathLength; i++) {
		if (DirectoryPath[i] < 0x20)
			return STATUS_INVALID_PATH;
		
		/*
			filters out any UTF-16 surrogate pairs.

			they are in the range of 0xD800 - 0xDFFF (first wchar)

			UCS-2 only, idk if this is the way for ntfs but we can move it to a fat32 path splitter.
		*/
		if ((DirectoryPath[i] & 0xFF00) >= 0xD800 && 
			(DirectoryPath[i] & 0xFF00) <= 0xDF00)
			return STATUS_INVALID_PATH;

		/*if (DirectoryPath[i] > 0x7E)
			return STATUS_INVALID_PATH;*/

		for (ULONG32 j = 0; j < (sizeof(FspIllegalCharacters)/sizeof(FspIllegalCharacters[0])); j++) {
			if (DirectoryPath[i] == FspIllegalCharacters[j])
				return STATUS_INVALID_PATH;
		}

		if (DirectoryPath[i] == '\\' &&
			DirectoryPath[i + 1] == 0)
			break;

		if (DirectoryPath[i] == '\\') {
			BufferSize += sizeof(void*);

			if (DirectoryPath[i + 1] == '\\')
				return STATUS_INVALID_PATH;
		}
	}

	*SplitPath = (PWCHAR*)ExAllocatePoolWithTag(BufferSize, 'htaP');

	ULONG32 j = 0;
	for (ULONG32 i = 0; i < PathLength; i++) {
		if (DirectoryPath[i] == '\\' &&
			DirectoryPath[i + 1] == 0) {
			DirectoryPath[i] = 0;
			break;
		}

		if (DirectoryPath[i] == '\\') {
			(*SplitPath)[j++] = &DirectoryPath[i + 1];
			DirectoryPath[i] = 0;
		}
	}
	(*SplitPath)[j] = NULL;

	return STATUS_SUCCESS;
}

