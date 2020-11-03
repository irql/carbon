


#include <carbsup.h>
#include "ldrp.h"
#include "pesup.h"
#include "ldrpusr.h"
#include "ldrpsup.h"
#include "ki_struct.h"
#include "psp.h"

NTSTATUS
LdrpUsrLoadModule(
	__in PKPROCESS ProcessObject,
	__in PUNICODE_STRING ModuleName
)
{

	ProcessObject;
	ModuleName;

	NTSTATUS ntStatus;
	IO_STATUS_BLOCK Iosb;


	HANDLE FileHandle;
	OBJECT_ATTRIBUTES FileAttributes = { 0, ModuleName };

	ntStatus = ZwCreateFile( &FileHandle, &Iosb, GENERIC_READ | GENERIC_WRITE, 0, &FileAttributes );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	if ( !NT_SUCCESS( Iosb.Status ) ) {

		return ntStatus;
	}

	FILE_BASIC_INFORMATION BasicInfo;

	ntStatus = ZwQueryInformationFile( FileHandle, &Iosb, &BasicInfo, sizeof( FILE_BASIC_INFORMATION ), FileBasicInformation );

	if ( !NT_SUCCESS( ntStatus ) ) {

		ZwClose( FileHandle );
		return ntStatus;
	}

	if ( !NT_SUCCESS( Iosb.Status ) ) {

		ZwClose( FileHandle );
		return Iosb.Status;
	}

	PVOID FileBase = MmAllocateMemory( BasicInfo.FileSize, PAGE_READ | PAGE_WRITE );

	ntStatus = ZwReadFile( FileHandle, &Iosb, FileBase, ( ULONG32 )BasicInfo.FileSize, 0 );

	if ( !NT_SUCCESS( ntStatus ) ) {

		MmFreeMemory( ( ULONG64 )FileBase, BasicInfo.FileSize );
		ZwClose( FileHandle );
		return ntStatus;
	}

	if ( !NT_SUCCESS( Iosb.Status ) ) {

		MmFreeMemory( ( ULONG64 )FileBase, BasicInfo.FileSize );
		ZwClose( FileHandle );
		return Iosb.Status;
	}

	PIMAGE_DOS_HEADER DosHeader = ( PIMAGE_DOS_HEADER )FileBase;

	if ( !PeSupVerifyDosHeader( DosHeader ) ) {

		MmFreeMemory( ( ULONG64 )FileBase, BasicInfo.FileSize );
		ZwClose( FileHandle );
		return STATUS_INVALID_PE_FILE;
	}

	PIMAGE_NT_HEADERS NtHeaders = ( PIMAGE_NT_HEADERS )( ( PCHAR )FileBase + DosHeader->e_lfanew );

	if ( !PeSupVerifyNtHeaders( NtHeaders ) ) {

		MmFreeMemory( ( ULONG64 )FileBase, BasicInfo.FileSize );
		ZwClose( FileHandle );
		return STATUS_INVALID_PE_FILE;
	}

	PIMAGE_SECTION_HEADER SectionHeaders = IMAGE_FIRST_SECTION64( NtHeaders );

	USHORT LastSection = NtHeaders->FileHeader.NumberOfSections - 1;
	ULONG64 ModuleSize = ( ULONG64 )SectionHeaders[ LastSection ].VirtualAddress + ( ULONG64 )ROUND_TO_PAGES( ( ( ULONG64 )SectionHeaders[ LastSection ].Misc.VirtualSize ) );
	PVOID ModuleBase = MmAllocateMemoryAtVirtual( NtHeaders->OptionalHeader.ImageBase, ModuleSize, PAGE_READ | PAGE_WRITE | PAGE_EXECUTE | PAGE_USER );

	if ( NtHeaders->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_BASERELOC ].VirtualAddress == 0 &&
		( ULONG64 )ModuleBase != NtHeaders->OptionalHeader.ImageBase ) {
		//lol imagine.

		MmFreeMemory( ( ULONG64 )ModuleBase, ModuleSize );
		MmFreeMemory( ( ULONG64 )FileBase, BasicInfo.FileSize );
		ZwClose( FileHandle );
		return STATUS_UNSUCCESSFUL;
	}

	_memcpy( ModuleBase, FileBase, NtHeaders->OptionalHeader.SizeOfHeaders );

	for ( USHORT i = 0; i < NtHeaders->FileHeader.NumberOfSections; i++ ) {

		_memset( ( PUCHAR )ModuleBase + SectionHeaders[ i ].VirtualAddress, 0, SectionHeaders[ i ].Misc.VirtualSize );
		_memcpy( ( PUCHAR )ModuleBase + SectionHeaders[ i ].VirtualAddress, ( PUCHAR )FileBase + SectionHeaders[ i ].PointerToRawData, SectionHeaders[ i ].SizeOfRawData );
	}

	if ( NtHeaders->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_BASERELOC ].VirtualAddress != 0 &&
		( ULONG64 )ModuleBase != NtHeaders->OptionalHeader.ImageBase ) {

		ntStatus = PeSupResolveBaseRelocDescriptor( ModuleBase, ( PIMAGE_BASE_RELOCATION )( ( PUCHAR )ModuleBase + NtHeaders->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_BASERELOC ].VirtualAddress ) );

		if ( !NT_SUCCESS( ntStatus ) ) {

			MmFreeMemory( ( ULONG64 )ModuleBase, ModuleSize );
			MmFreeMemory( ( ULONG64 )FileBase, BasicInfo.FileSize );
			ZwClose( FileHandle );
			return STATUS_UNSUCCESSFUL;
		}
	}

	if ( ProcessObject->VadTree.Range.ModuleStart == NULL ) {
		// insert as the first vad.

		ntStatus = LdrpSupGetInfoBlock( ModuleBase, &ProcessObject->VadTree.Range );

		if ( !NT_SUCCESS( ntStatus ) ) {

			ProcessObject->VadTree.Range.ModuleStart = NULL;
			MmFreeMemory( ( ULONG64 )ModuleBase, ModuleSize );
			MmFreeMemory( ( ULONG64 )FileBase, BasicInfo.FileSize );
			ZwClose( FileHandle );
			return STATUS_UNSUCCESSFUL;
		}

		RtlAllocateAndInitUnicodeString( &ProcessObject->VadTree.RangeName, LdrpNameFromPath( ModuleName->Buffer ) );

		ProcessObject->VadTree.Next = NULL;
	}
	else {
		// find last vad and insert.

		PVAD Vad = PspAllocateVad( );
		ntStatus = LdrpSupGetInfoBlock( ModuleBase, &Vad->Range );

		if ( !NT_SUCCESS( ntStatus ) ) {

			MmFreeMemory( ( ULONG64 )ModuleBase, ModuleSize );
			MmFreeMemory( ( ULONG64 )FileBase, BasicInfo.FileSize );
			ZwClose( FileHandle );
			return STATUS_UNSUCCESSFUL;
		}

		RtlAllocateAndInitUnicodeString( &Vad->RangeName, LdrpNameFromPath( ModuleName->Buffer ) );

		PspInsertVad( ProcessObject, Vad );
	}

	if ( NtHeaders->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_IMPORT ].VirtualAddress != 0 ) {

		PIMAGE_IMPORT_DESCRIPTOR Import = ( PIMAGE_IMPORT_DESCRIPTOR )( ( PUCHAR )ModuleBase + NtHeaders->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_IMPORT ].VirtualAddress );

		PWCHAR ModuleFileNameBuffer = ExAllocatePoolWithTag( 256 * sizeof( WCHAR ), TAGEX_STRING );

		ntStatus = STATUS_SUCCESS;

		while ( Import->Characteristics ) {

			if ( !NT_SUCCESS( ntStatus ) ) {

				// impl some proper cleanup code.
				MmFreeMemory( ( ULONG64 )ModuleBase, ModuleSize );
				MmFreeMemory( ( ULONG64 )FileBase, BasicInfo.FileSize );
				ZwClose( FileHandle );
				return ntStatus;
			}

			for ( ULONG32 i = 0; ( ( PCHAR )( ( ULONG64 )ModuleBase + Import->Name ) )[ i ]; i++ ) {

				ModuleFileNameBuffer[ i ] = ( ( PCHAR )( ( ULONG64 )ModuleBase + Import->Name ) )[ i ];
				ModuleFileNameBuffer[ i + 1 ] = 0;
			}

			PVAD CurrentVad = PspFindVad( ProcessObject, ModuleFileNameBuffer );

			if ( CurrentVad != NULL ) {

				ntStatus = PeSupResolveImportDescriptorSingle( ModuleBase, CurrentVad->Range.ModuleStart, Import );

				Import++;
				continue;
			}
			else {

				UNICODE_STRING NextFileName;

				NextFileName.Size = ( 12 + lstrlenW( ModuleFileNameBuffer ) + 1 ) * sizeof( WCHAR );
				NextFileName.Buffer = ExAllocatePoolWithTag( NextFileName.Size, TAGEX_STRING );

				lstrcpyW( NextFileName.Buffer, L"\\SystemRoot\\" );
				lstrcatW( NextFileName.Buffer, ModuleFileNameBuffer );

				NextFileName.Length = lstrlenW( NextFileName.Buffer );

				ntStatus = LdrpUsrLoadModule( ProcessObject, &NextFileName );

				continue;
			}
		}
	}



	return STATUS_SUCCESS;
}
