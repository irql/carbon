/*++

Module ObjectName:

	pesup.c

Abstract:

	Portable Executable format binaries, supervisor.

--*/

#include <carbsup.h>
#include "pesup.h"
#include "mi.h"
#include "obp.h"
#include "ldrpsup.h"

NTSTATUS
PeSupGetProcedureAddressByName(
	__in PVOID ModuleBase,
	__in CHAR* ExportName,
	__out PVOID* ProcedureAddress
)
/*++

Routine Description:

	Abc.

Arguments:

	.

Return Value:

	None.

--*/
{
	IMAGE_DOS_HEADER* DosHeader = ( IMAGE_DOS_HEADER* )ModuleBase;

	if ( !PeSupVerifyDosHeader( DosHeader ) ) {

		return STATUS_UNSUCCESSFUL;
	}

	IMAGE_NT_HEADERS* NtHeaders = ( IMAGE_NT_HEADERS* )( ( char* )ModuleBase + DosHeader->e_lfanew );

	if ( !PeSupVerifyNtHeaders( NtHeaders ) ) {

		return STATUS_UNSUCCESSFUL;
	}

	if ( NtHeaders->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXPORT ].VirtualAddress == 0 ||
		NtHeaders->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXPORT ].Size == 0 ) {

		return STATUS_UNSUCCESSFUL;
	}

	PIMAGE_EXPORT_DIRECTORY ExportDirectory = ( PIMAGE_EXPORT_DIRECTORY )( ( PUCHAR )ModuleBase + NtHeaders->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXPORT ].VirtualAddress );

	ULONG* Functions = ( ULONG* )( ( PUCHAR )ModuleBase + ExportDirectory->AddressOfFunctions );
	ULONG* Names = ( ULONG* )( ( PUCHAR )ModuleBase + ExportDirectory->AddressOfNames );
	USHORT* Ordinals = ( USHORT* )( ( PUCHAR )ModuleBase + ExportDirectory->AddressOfNameOrdinals );

	for ( ULONG i = 0; i < ExportDirectory->NumberOfNames; i++ ) {
		CHAR* Name = ( char* )ModuleBase + Names[ i ];

		if ( _strcmp( Name, ExportName ) == 0 ) {

			*ProcedureAddress = ( PVOID )( ( PUCHAR )ModuleBase + Functions[ Ordinals[ i ] ] );

			return STATUS_SUCCESS;
		}
	}

	return STATUS_NOT_FOUND;
}

NTSTATUS
PeSupGetProcedureAddressByOrdinal(
	__in PVOID ModuleBase,
	__in USHORT ExportOrdinal,
	__out PVOID* ProcedureAddress
)
/*++

Routine Description:

	Abc.

Arguments:

	.

Return Value:

	None.

--*/
{
	PIMAGE_DOS_HEADER DosHeader = ( PIMAGE_DOS_HEADER )ModuleBase;

	if ( !PeSupVerifyDosHeader( DosHeader ) ) {

		return STATUS_UNSUCCESSFUL;
	}

	IMAGE_NT_HEADERS* NtHeaders = ( IMAGE_NT_HEADERS* )( ( char* )ModuleBase + DosHeader->e_lfanew );

	if ( !PeSupVerifyNtHeaders( NtHeaders ) ) {

		return STATUS_UNSUCCESSFUL;
	}

	if ( NtHeaders->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXPORT ].VirtualAddress == 0 ||
		NtHeaders->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXPORT ].Size == 0 ) {

		return STATUS_UNSUCCESSFUL;
	}

	PIMAGE_EXPORT_DIRECTORY ExportDirectory = ( PIMAGE_EXPORT_DIRECTORY )( ( PUCHAR )ModuleBase + NtHeaders->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXPORT ].VirtualAddress );

	ULONG* Functions = ( ULONG* )( ( PUCHAR )ModuleBase + ExportDirectory->AddressOfFunctions );
	USHORT* Ordinals = ( USHORT* )( ( PUCHAR )ModuleBase + ExportDirectory->AddressOfNameOrdinals );

	for ( ULONG i = 0; i < ExportDirectory->NumberOfFunctions; i++ ) {
		USHORT Ordinal = *( PUSHORT )( ( PUCHAR )ModuleBase + Ordinals[ i ] );

		if ( Ordinal == ExportOrdinal ) {

			*ProcedureAddress = ( PVOID )( ( PUCHAR )ModuleBase + Functions[ Ordinals[ i ] ] );

			return STATUS_SUCCESS;
		}
	}

	return STATUS_NOT_FOUND;
}

NTSTATUS
PeSupResolveImportDescriptorSingle(
	__in PVOID ModuleBase,
	__in PVOID DescribedModuleBase,
	__in PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor
)
/*++

Routine Description:

	Resolves imports for a descriptor.

Arguments:

	ModuleBase - Supplies a pointer to the module we are resolving.

	DescribedModuleBase - Supplies a pointer to the base address of the described module,
		in the import descriptor.

	ImportDescriptor - Supplies a pointer to the import descriptor relative to the ModuleBase.

Return Value:

	Status.

--*/
{
	NTSTATUS ntStatus;

	PIMAGE_THUNK_DATA OriginalFirstThunk = ( PIMAGE_THUNK_DATA )( ( char* )ModuleBase + ImportDescriptor->OriginalFirstThunk );
	PIMAGE_THUNK_DATA FirstThunk = ( PIMAGE_THUNK_DATA )( ( char* )ModuleBase + ImportDescriptor->FirstThunk );

	while ( OriginalFirstThunk->u1.AddressOfData ) {
		if ( OriginalFirstThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG64 ) {

			ntStatus = PeSupGetProcedureAddressByOrdinal( DescribedModuleBase, ( USHORT )( OriginalFirstThunk->u1.Ordinal & 0xffff ), ( PVOID* )&FirstThunk->u1.Function );

			if ( !NT_SUCCESS( ntStatus ) ) {

				return ntStatus;

			}
		}
		else {
			PIMAGE_IMPORT_BY_NAME ImportByName = ( PIMAGE_IMPORT_BY_NAME )( ( char* )ModuleBase + OriginalFirstThunk->u1.AddressOfData );

			ntStatus = PeSupGetProcedureAddressByName( DescribedModuleBase, ( CHAR* )ImportByName->Name, ( PVOID* )&FirstThunk->u1.Function );

			if ( !NT_SUCCESS( ntStatus ) ) {

				return ntStatus;
			}
		}
		OriginalFirstThunk++;
		FirstThunk++;
	}

	return STATUS_SUCCESS;
}

NTSTATUS
PeSupResolveBaseRelocDescriptor(
	__in PVOID ModuleBase,
	__in PIMAGE_BASE_RELOCATION BaseRelocation
)
{

	IMAGE_DOS_HEADER* DosHeader = ( IMAGE_DOS_HEADER* )ModuleBase;

	if ( !PeSupVerifyDosHeader( DosHeader ) ) {

		return STATUS_UNSUCCESSFUL;
	}

	IMAGE_NT_HEADERS* NtHeaders = ( IMAGE_NT_HEADERS* )( ( char* )ModuleBase + DosHeader->e_lfanew );

	if ( !PeSupVerifyNtHeaders( NtHeaders ) ) {

		return STATUS_UNSUCCESSFUL;
	}

	LONG64 Delta = ( LONG64 )( ( PUCHAR )ModuleBase - NtHeaders->OptionalHeader.ImageBase );

	if ( Delta == 0 ) {

		//stupid cunt
		return STATUS_SUCCESS;
	}

	while ( BaseRelocation->VirtualAddress ) {

		if ( BaseRelocation->SizeOfBlock > sizeof( IMAGE_BASE_RELOCATION ) ) {
			ULONG64 RelocCount = ( BaseRelocation->SizeOfBlock - sizeof( IMAGE_BASE_RELOCATION ) ) / sizeof( USHORT );
			PUSHORT RelocList = ( PUSHORT )( ( PUCHAR )BaseRelocation + sizeof( IMAGE_BASE_RELOCATION ) );

			for ( ULONG64 i = 0; i < RelocCount; i++ ) {

				if ( RelocList[ i ] ) {
					PLONG64 AbsoluteAddress = ( PLONG64 )( ( PUCHAR )ModuleBase + ( BaseRelocation->VirtualAddress + ( RelocList[ i ] & 0xfff ) ) );
					*AbsoluteAddress += Delta;
				}
			}
		}

		BaseRelocation = ( PIMAGE_BASE_RELOCATION )( ( PUCHAR )BaseRelocation + BaseRelocation->SizeOfBlock );
	}

	return STATUS_SUCCESS;
}

