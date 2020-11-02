/*++

Module ObjectName:

	ldrsup.c

Abstract:



--*/

#include <carbsup.h>
#include "pesup.h"
#include "ldrsup.h"
#include "mi.h"
#include "ldr.h"

NTSTATUS
LdrpSupGetInfoBlock(
	__in PVOID ModuleBase,
	__in PLDR_INFO_BLOCK InfoBlock
)
/*++

Routine Description:

	Supplies a LDR_INFO_BLOCK for a specified module.

Arguments:

	ModuleBase - Supplies a pointer to the base of the module to create a loader info block for.

	InfoBlock - Supplies a pointer for the procedure to write the loader info block.

Return Value:

	Status.

--*/
{
	PIMAGE_DOS_HEADER DosHeader = ( PIMAGE_DOS_HEADER )ModuleBase;

	if ( !PeSupVerifyDosHeader( DosHeader ) ) {

		return STATUS_INVALID_PE_FILE;
	}

	PIMAGE_NT_HEADERS NtHeaders = ( PIMAGE_NT_HEADERS )( ( char* )ModuleBase + DosHeader->e_lfanew );

	if ( !PeSupVerifyNtHeaders( NtHeaders ) ) {

		return STATUS_INVALID_PE_FILE;
	}

	PIMAGE_SECTION_HEADER SectionHeaders = IMAGE_FIRST_SECTION64( NtHeaders );

	USHORT LastSection = NtHeaders->FileHeader.NumberOfSections - 1;
	ULONG64 ModuleSize = ( ULONG64 )SectionHeaders[ LastSection ].VirtualAddress + ( ULONG64 )ROUND_TO_PAGES( ( ( ULONG64 )SectionHeaders[ LastSection ].Misc.VirtualSize ) );

	InfoBlock->ModuleStart = ModuleBase;
	InfoBlock->ModuleEnd = ( char* )ModuleBase + ModuleSize;
	InfoBlock->ModuleEntry = ( char* )ModuleBase + NtHeaders->OptionalHeader.AddressOfEntryPoint;

	return STATUS_SUCCESS;
}

NTSTATUS
LdrpSupLoadModule(
	__in PVOID FileBase,
	__in PLDR_INFO_BLOCK InfoBlock
)
{

	PIMAGE_DOS_HEADER DosHeader = ( PIMAGE_DOS_HEADER )FileBase;

	if ( !PeSupVerifyDosHeader( DosHeader ) ) {

		return STATUS_INVALID_PE_FILE;
	}

	PIMAGE_NT_HEADERS NtHeaders = ( PIMAGE_NT_HEADERS )( ( char* )FileBase + DosHeader->e_lfanew );

	if ( !PeSupVerifyNtHeaders( NtHeaders ) ) {

		return STATUS_INVALID_PE_FILE;
	}

	NTSTATUS ntStatus;

	PIMAGE_SECTION_HEADER SectionHeaders = IMAGE_FIRST_SECTION64( NtHeaders );

	USHORT LastSection = NtHeaders->FileHeader.NumberOfSections - 1;
	ULONG64 ModuleSize = ( ULONG64 )SectionHeaders[ LastSection ].VirtualAddress + ( ULONG64 )ROUND_TO_PAGES( ( ( ULONG64 )SectionHeaders[ LastSection ].Misc.VirtualSize ) );
	PVOID ModuleBase = NULL;

	ModuleBase = MmAllocateMemory( ModuleSize, PAGE_READ | PAGE_WRITE | PAGE_EXECUTE );

	_memcpy( ModuleBase, FileBase, NtHeaders->OptionalHeader.SizeOfHeaders );

	for ( USHORT i = 0; i < NtHeaders->FileHeader.NumberOfSections; i++ ) {

		PVOID SectionBase = ( PUCHAR )ModuleBase + SectionHeaders[ i ].VirtualAddress;
		_memcpy( SectionBase, ( PUCHAR )FileBase + SectionHeaders[ i ].PointerToRawData, SectionHeaders[ i ].SizeOfRawData );
	}

	ntStatus = LdrpSupGetInfoBlock( ModuleBase, InfoBlock );

	if ( !NT_SUCCESS( ntStatus ) ) {

		MmFreeMemory( ( ULONG64 )ModuleBase, ModuleSize );
		return ntStatus;
	}

	if ( NtHeaders->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_BASERELOC ].VirtualAddress != 0 ) {

		ntStatus = PeSupResolveBaseRelocDescriptor( ModuleBase,
			( PIMAGE_BASE_RELOCATION )( ( PUCHAR )ModuleBase + NtHeaders->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_BASERELOC ].VirtualAddress ) );

		if ( !NT_SUCCESS( ntStatus ) ) {

			MmFreeMemory( ( ULONG64 )ModuleBase, ModuleSize );
			return ntStatus;
		}

	}

	if ( NtHeaders->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_IMPORT ].VirtualAddress ) {
		//this function is only for the disk driver and that driver should only want kernel.sys

		PIMAGE_IMPORT_DESCRIPTOR iat = ( PIMAGE_IMPORT_DESCRIPTOR )( ( PUCHAR )ModuleBase + NtHeaders->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_IMPORT ].VirtualAddress );


#if 1
		ntStatus = PeSupResolveImportDescriptorSingle( ModuleBase, ( PVOID )KERNEL_IMAGE_BASE, iat );

		if ( !NT_SUCCESS( ntStatus ) ) {

			MmFreeMemory( ( ULONG64 )ModuleBase, ModuleSize );
			return ntStatus;
		}
#endif
	}

	return STATUS_SUCCESS;
}

NTSTATUS
LdrSupLoadSupervisorModule(
	__in HANDLE FileHandle,
	__in PLDR_INFO_BLOCK InfoBlock
)
{
	NTSTATUS ntStatus;
	IO_STATUS_BLOCK Iosb;
	FILE_BASIC_INFORMATION BasicInfo;

	ntStatus = ZwQueryInformationFile( FileHandle, &Iosb, &BasicInfo, sizeof( FILE_BASIC_INFORMATION ), FileBasicInformation );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	if ( !NT_SUCCESS( Iosb.Status ) ) {

		return Iosb.Status;
	}

	PVOID FileBase = MmAllocateMemory( BasicInfo.FileSize, PAGE_READ | PAGE_WRITE );

	ntStatus = ZwReadFile( FileHandle, &Iosb, FileBase, ( ULONG32 )BasicInfo.FileSize, 0 );

	if ( !NT_SUCCESS( ntStatus ) ) {

		MmFreeMemory( ( ULONG64 )FileBase, BasicInfo.FileSize );
		return ntStatus;
	}

	if ( !NT_SUCCESS( Iosb.Status ) ) {

		MmFreeMemory( ( ULONG64 )FileBase, BasicInfo.FileSize );
		return Iosb.Status;
	}

	PIMAGE_DOS_HEADER DosHeader = ( PIMAGE_DOS_HEADER )FileBase;

	if ( !PeSupVerifyDosHeader( DosHeader ) ) {

		MmFreeMemory( ( ULONG64 )FileBase, BasicInfo.FileSize );
		return STATUS_INVALID_PE_FILE;
	}

	PIMAGE_NT_HEADERS NtHeaders = ( PIMAGE_NT_HEADERS )( ( PCHAR )FileBase + DosHeader->e_lfanew );

	if ( !PeSupVerifyNtHeaders( NtHeaders ) ) {

		MmFreeMemory( ( ULONG64 )FileBase, BasicInfo.FileSize );
		return STATUS_INVALID_PE_FILE;
	}

	PIMAGE_SECTION_HEADER SectionHeaders = IMAGE_FIRST_SECTION64( NtHeaders );

	USHORT LastSection = NtHeaders->FileHeader.NumberOfSections - 1;
	ULONG64 ModuleSize = ( ULONG64 )SectionHeaders[ LastSection ].VirtualAddress + ( ULONG64 )ROUND_TO_PAGES( ( ( ULONG64 )SectionHeaders[ LastSection ].Misc.VirtualSize ) );
	PVOID ModuleBase = NULL;

#if 0
	ntStatus = LdrpSupFindModuleBase( FileBase, &ModuleBase );

	if ( !NT_SUCCESS( ntStatus ) ) {

		MmFreeMemory( ( ULONG64 )FileBase, BasicInfo.FileSize );
		return ntStatus;
	}

	ModuleBase = ( PVOID )MmAllocateMemoryAtVirtual( ( ULONG64 )ModuleBase, ModuleSize, PAGE_READ | PAGE_WRITE | PAGE_EXECUTE );
#endif

	ModuleBase = MmAllocateMemory( ModuleSize, PAGE_READ | PAGE_WRITE | PAGE_EXECUTE );

	_memcpy( ModuleBase, FileBase, NtHeaders->OptionalHeader.SizeOfHeaders );

	for ( USHORT i = 0; i < NtHeaders->FileHeader.NumberOfSections; i++ ) {

		PVOID SectionBase = ( PUCHAR )ModuleBase + SectionHeaders[ i ].VirtualAddress;
		_memcpy( SectionBase, ( PUCHAR )FileBase + SectionHeaders[ i ].PointerToRawData, SectionHeaders[ i ].SizeOfRawData );
	}

	ntStatus = LdrpSupGetInfoBlock( ModuleBase, InfoBlock );

	if ( !NT_SUCCESS( ntStatus ) ) {

		MmFreeMemory( ( ULONG64 )FileBase, BasicInfo.FileSize );
		MmFreeMemory( ( ULONG64 )ModuleBase, ModuleSize );
		return ntStatus;
	}

	if ( NtHeaders->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_BASERELOC ].VirtualAddress != 0 ) {

		ntStatus = PeSupResolveBaseRelocDescriptor( ModuleBase,
			( PIMAGE_BASE_RELOCATION )( ( PUCHAR )ModuleBase + NtHeaders->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_BASERELOC ].VirtualAddress ) );

		if ( !NT_SUCCESS( ntStatus ) ) {

			MmFreeMemory( ( ULONG64 )FileBase, BasicInfo.FileSize );
			MmFreeMemory( ( ULONG64 )ModuleBase, ModuleSize );
			return ntStatus;
		}

	}

	if ( NtHeaders->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_IMPORT ].VirtualAddress ) {
		//UNIMPLEMENTED.
		//write ZwQueryObjectInformation & Object

		PIMAGE_IMPORT_DESCRIPTOR iat = ( PIMAGE_IMPORT_DESCRIPTOR )( ( PUCHAR )ModuleBase + NtHeaders->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_IMPORT ].VirtualAddress );
#if 1
		PWCHAR ModuleFileNameBuffer = ExAllocatePoolWithTag( 256 * sizeof( WCHAR ), TAGEX_FILE );

		while ( iat->Characteristics ) {


			for ( ULONG32 i = 0; ( ( PCHAR )( ( ULONG64 )ModuleBase + iat->Name ) )[ i ]; i++ ) {

				ModuleFileNameBuffer[ i ] = ( ( PCHAR )( ( ULONG64 )ModuleBase + iat->Name ) )[ i ];
				ModuleFileNameBuffer[ i + 1 ] = 0;
			}

			PLIST_ENTRY Flink = ObjectTypeModule->ObjectList.List;
			ntStatus = STATUS_NOT_FOUND;

			do {
				POBJECT_ENTRY_HEADER Module = CONTAINING_RECORD( Flink, OBJECT_ENTRY_HEADER, ObjectList );
				PKMODULE ModuleObject = ( PKMODULE )( Module + 1 );

				if ( ModuleObject->ImageName.Buffer == NULL ) {

					continue;
				}

				if ( RtlStringCompare( ModuleFileNameBuffer, LdrpNameFromPath( ModuleObject->ImageName.Buffer ) ) == 0 ) {

					ntStatus = PeSupResolveImportDescriptorSingle( ModuleBase, ModuleObject->LoaderInfoBlock.ModuleStart, iat );
					break;
				}

				Flink = Flink->Flink;
			} while ( Flink != ObjectTypeModule->ObjectList.List );

			if ( !NT_SUCCESS( ntStatus ) ) {

				ExFreePoolWithTag( ModuleFileNameBuffer, TAGEX_FILE );
				MmFreeMemory( ( ULONG64 )FileBase, BasicInfo.FileSize );
				MmFreeMemory( ( ULONG64 )ModuleBase, ModuleSize );
				return ntStatus;
			}

			iat++;
		}

		ExFreePoolWithTag( ModuleFileNameBuffer, TAGEX_FILE );
#else
		ntStatus = PeSupResolveImportDescriptorSingle( ModuleBase, ( PVOID )KERNEL_IMAGE_BASE, iat );


		if ( !NT_SUCCESS( ntStatus ) ) {

			MmFreeMemory( ( ULONG64 )FileBase, BasicInfo.FileSize );
			MmFreeMemory( ( ULONG64 )ModuleBase, ModuleSize );
			return ntStatus;
		}
#endif
	}

	MmFreeMemory( ( ULONG64 )FileBase, BasicInfo.FileSize );

	return STATUS_SUCCESS;
}