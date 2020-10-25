

#include "fat32.h"

NTSTATUS
FsFat32ReadFile(
	__in PFAT32FS FileSystem,
	__in PFAT32_DIRECTORY_ENTRY DirectoryFile,
	__in PWCHAR FileName,
	__in PFILE_OBJECT FileObject
	)
{

	FAT32_PATH_TYPE PathType = FsFat32VerifyFileName(FileName);

	if (PathType == Fat32PathInvalid) {

		return STATUS_INVALID_PATH;
	}
	
	if (PathType == Fat32Path8Dot3) {

		CHAR Fat32FileName[12];
		FsFat32ConvertPathTo8Dot3(FileName, Fat32FileName);

		FileName = (PWCHAR)&Fat32FileName;

		
	}
	else if (PathType == Fat32PathLongFileName) {


	}

	ULONG64 Fat32File = FsFat32FindFile(DirectoryFile, PathType, FileName);

	if (Fat32File == (ULONG64)-1) {

		return STATUS_NOT_FOUND;
	}

	if (DirectoryFile[Fat32File].Attributes & FAT32_DIRECTORY)
		FileObject->Flags |= FILE_FLAG_DIRECTORY;

	if (DirectoryFile[Fat32File].Attributes & FAT32_HIDDEN)
		FileObject->Flags |= FILE_FLAG_ATTRIBUTE_HIDDEN;
	if (DirectoryFile[Fat32File].Attributes & FAT32_SYSTEM)
		FileObject->Flags |= FILE_FLAG_ATTRIBUTE_SYSTEM;
	if (DirectoryFile[Fat32File].Attributes & FAT32_READ_ONLY)
		FileObject->Flags |= FILE_FLAG_ATTRIBUTE_READONLY;

	FileObject->FileSize = (ULONG64)DirectoryFile[Fat32File].FileSize;

	ULONG32 StartCluster = (((ULONG32)DirectoryFile[Fat32File].ClusterHigh << 16) | (ULONG32)DirectoryFile[Fat32File].ClusterLow);
	ULONG32 CurrentCluster = StartCluster;
	ULONG64 SectorCount = 0;

	do {
		SectorCount += FileSystem->Bpb->Dos2_00Bpb.SectorsPerCluster;

		CurrentCluster = FspFat32QueryFatTable(FileSystem, CurrentCluster);
	} while (CurrentCluster != FAT32_END_OF_CHAIN);
	CurrentCluster = StartCluster;

	FileObject->FileMemorySize = (ULONG64)SectorCount * (ULONG64)FileSystem->Bpb->Dos2_00Bpb.BytesPerSector;
	FileObject->FileMemory = (PVOID)MmAllocateMemory(FileObject->FileMemorySize, PAGE_READ | PAGE_WRITE);

	ULONG32 ReadClusters = 0;
	NTSTATUS ntStatus;

	do {
		ntStatus = FileSystem->Drive->Read(FileSystem->Access, FIRST_SECTOR_OF_CLUSTER(FileSystem->Bpb, CurrentCluster), 
			(((UCHAR*)(FileObject->FileMemory)) + ((ULONG64)ReadClusters * 
			((ULONG64)FileSystem->Bpb->Dos2_00Bpb.SectorsPerCluster * 
				(ULONG64)FileSystem->Bpb->Dos2_00Bpb.BytesPerSector))), 
			(ULONG64)FileSystem->Bpb->Dos2_00Bpb.SectorsPerCluster *
			(ULONG64)FileSystem->Bpb->Dos2_00Bpb.BytesPerSector);

		if (!NT_SUCCESS(ntStatus)) {

			MmFreeMemory((ULONG64)FileObject->FileMemory, FileObject->FileMemorySize);
			return ntStatus;
		}

		ReadClusters++;
		CurrentCluster = FspFat32QueryFatTable(FileSystem, CurrentCluster);
	} while (CurrentCluster != FAT32_END_OF_CHAIN);

	return STATUS_SUCCESS;
}

NTSTATUS
FsFat32ReadDirectoryFile(
	__in PPARTITION_OBJECT Partition,
	__in PFILE_OBJECT FileObject,
	__in PIO_STATUS_BLOCK UserIosb
	)
{
	Partition;

	PWCHAR* DirectoryPathParts;
	UserIosb->Status = FsSplitDirectoryPath(FileObject->FsFileName->Buffer, &DirectoryPathParts);

	if (!NT_SUCCESS(UserIosb->Status)) {

		return UserIosb->Status;
	}

	for (ULONG32 i = 0; DirectoryPathParts[i] != NULL; i++) {

		if (FsFat32VerifyFileName(DirectoryPathParts[i]) == Fat32PathInvalid) {

			ExFreePoolWithTag(DirectoryPathParts, 'htaP');

			UserIosb->Status = STATUS_INVALID_PATH;
			return UserIosb->Status;
		}
	}

	FAT32FS Fs;
	Fs.Access = Partition;
	Fs.Drive = &Partition->AccessBlock;
	Fs.Bpb = Partition->BootSector;

	FileObject->DirectoryFile->FileMemorySize = (ULONG64)Fs.Bpb->Dos2_00Bpb.SectorsPerCluster * (ULONG64)Fs.Bpb->Dos2_00Bpb.BytesPerSector;
	FileObject->DirectoryFile->FileMemory = (PFAT32_DIRECTORY_ENTRY)MmAllocateMemory(FileObject->DirectoryFile->FileMemorySize, PAGE_READ | PAGE_WRITE);

	UserIosb->Status = Fs.Drive->Read(Fs.Access, FIRST_SECTOR_OF_CLUSTER(Fs.Bpb, Fs.Bpb->Dos7_01Bpb.RootDirectoryCluster), FileObject->DirectoryFile->FileMemory, (ULONG32)FileObject->DirectoryFile->FileMemorySize);

	if (!NT_SUCCESS(UserIosb->Status)) {
		ExFreePoolWithTag(DirectoryPathParts, 'htaP');
		MmFreeMemory((ULONG64)FileObject->DirectoryFile->FileMemory, FileObject->DirectoryFile->FileMemorySize);

		return UserIosb->Status;
	}

	PFAT32_DIRECTORY_ENTRY DirectoryEntry = FileObject->DirectoryFile->FileMemory;
	ULONG64 DirectoryEntrySize = FileObject->DirectoryFile->FileMemorySize;

	FileObject->FileMemory = DirectoryEntry;
	FileObject->FileMemorySize = DirectoryEntrySize;

	for (ULONG32 i = 0; DirectoryPathParts[i] != NULL; i++) {

		if (DirectoryPathParts[i + 1] == NULL) {

			UserIosb->Status = FsFat32ReadFile(&Fs, (PFAT32_DIRECTORY_ENTRY)DirectoryEntry, DirectoryPathParts[i], FileObject);
		}
		else {
			UserIosb->Status = FsFat32ReadFile(&Fs, (PFAT32_DIRECTORY_ENTRY)DirectoryEntry, DirectoryPathParts[i], FileObject->DirectoryFile);

		}

		if (!NT_SUCCESS(UserIosb->Status)) {
			ExFreePoolWithTag(DirectoryPathParts, 'htaP');
			MmFreeMemory((ULONG64)FileObject->DirectoryFile->FileMemory, FileObject->DirectoryFile->FileMemorySize);

			return UserIosb->Status;
		}

		if (DirectoryPathParts[i + 1] != NULL) {
			
			MmFreeMemory((ULONG64)DirectoryEntry, DirectoryEntrySize);
			DirectoryEntry = (PFAT32_DIRECTORY_ENTRY)FileObject->DirectoryFile->FileMemory;
			DirectoryEntrySize = FileObject->DirectoryFile->FileMemorySize;
		}
	}

	UserIosb->Status = STATUS_SUCCESS;
	UserIosb->Information = FILE_OPENED;
	
	return STATUS_SUCCESS;
}
