/*++

Module ObjectName:

	acpi.c

Abstract:

	!

--*/

#include <carbsup.h>
#include "hal.h"
#include "acpi.h"
#include "mm.h"

PRSDP_DESCRIPTOR2_0						Rsdp;
PRSDT_DESCRIPTOR						Rsdt;
PXSDT_DESCRIPTOR						Xsdt;
PMADT									Madt;

PMADT_PROCESSOR_LOCAL_APIC*				MadtProcessorLocalApics;
PMADT_IO_APIC*							MadtIoApics;
PMADT_INTERRUPT_SOURCE_OVERRIDE_APIC*	MadtInterruptSourceOverrideApics;
PMADT_NON_MASKABLE_INTERRUPT_APIC*		MadtNonMaskableInterruptApics;

ULONG									MadtProcessorLocalApicCount = 0;
ULONG									MadtIoApicCount = 0;
ULONG									MadtInterruptSourceOverrideApicCount = 0;
ULONG									MadtNonMaskableInterruptApicCount = 0;

PMADT_LOCAL_APIC_ADDRESS_OVERRIDE		MadtLocalApicAddressOverride;

PSDT_HEADER
HalAcpiFindSdt(
	__in PCHAR Signature
)
{

	if ( Xsdt != NULL ) {
		ULONG32 Entries = ( Xsdt->Header.Length - sizeof( SDT_HEADER ) ) / 8;

		for ( ULONG32 i = 0; i < Entries; i++ ) {
			//PSDT_HEADER Header = (PSDT_HEADER)MmpFindFreeVirtual(0x1000, PAGE_READ | PAGE_WRITE);
			//MmAllocateMemoryAtPhysical((ULONG64)(Xsdt->SdtPointers[i] & ~0xfff), (ULONG64)Header, 0x1000, PAGE_READ | PAGE_WRITE);
			PSDT_HEADER Header = MmAllocateMemoryAtPhysical( ( ULONG64 )( Xsdt->SdtPointers[ i ] & ~0xfff ), 0x1000, PAGE_READ | PAGE_WRITE );
			Header = ( PSDT_HEADER )( ( char* )Header + ( Xsdt->SdtPointers[ i ] & 0xfff ) );

			if ( Header->Length > ( 0x1000 - ( Xsdt->SdtPointers[ i ] & 0xfff ) ) ) {
				ULONG32 Length = Header->Length;

				MmFreeMemory( ( ( ULONG64 )Header ) & ~0xfff, 0x1000 );

				//Header = ( PSDT_HEADER )MmpFindFreeVirtual( Length, PAGE_READ | PAGE_WRITE );
				//MmAllocateMemoryAtPhysical( ( ULONG64 )( Xsdt->SdtPointers[ i ] & ~0xfff ), ( ULONG64 )Header, Length, PAGE_READ | PAGE_WRITE );
				Header = MmAllocateMemoryAtPhysical( ( ULONG64 )( Xsdt->SdtPointers[ i ] & ~0xfff ),
					Length + ( Xsdt->SdtPointers[ i ] & 0xfff ), PAGE_READ | PAGE_WRITE );
				Header = ( PSDT_HEADER )( ( char* )Header + ( Xsdt->SdtPointers[ i ] & 0xfff ) );
			}

			if ( _memcmp( Header->Signature, Signature, 4 ) == 0 )
				return Header;

			MmFreeMemory( ( ULONG64 )Header, Header->Length );
		}
	}
	else {
		ULONG32 Entries = ( Rsdt->Header.Length - sizeof( SDT_HEADER ) ) / 4;

		for ( ULONG32 i = 0; i < Entries; i++ ) {
			//PSDT_HEADER Header = ( PSDT_HEADER )MmpFindFreeVirtual( 0x1000, PAGE_READ | PAGE_WRITE );
			//MmAllocateMemoryAtPhysical( ( ULONG64 )( Rsdt->SdtPointers[ i ] & ~0xfff ), ( ULONG64 )Header, 0x1000, PAGE_READ | PAGE_WRITE );
			PSDT_HEADER Header = MmAllocateMemoryAtPhysical( ( ULONG64 )( Rsdt->SdtPointers[ i ] & ~0xfff ), 0x1000, PAGE_READ | PAGE_WRITE );
			Header = ( PSDT_HEADER )( ( char* )Header + ( Rsdt->SdtPointers[ i ] & 0xfff ) );

			if ( Header->Length > ( 0x1000 - ( Rsdt->SdtPointers[ i ] & 0xfff ) ) ) {
				ULONG32 Length = Header->Length;

				MmFreeMemory( ( ( ULONG64 )Header ) & ~0xfff, 0x1000 );

				//Header = ( PSDT_HEADER )MmpFindFreeVirtual( Length, PAGE_READ | PAGE_WRITE );
				//MmAllocateMemoryAtPhysical( ( ULONG64 )( Rsdt->SdtPointers[ i ] & ~0xfff ), ( ULONG64 )Header, Length, PAGE_READ | PAGE_WRITE );
				Header = MmAllocateMemoryAtPhysical( ( ULONG64 )( Rsdt->SdtPointers[ i ] & ~0xfff ),
					Length + ( Rsdt->SdtPointers[ i ] & 0xfff ), PAGE_READ | PAGE_WRITE );
				Header = ( PSDT_HEADER )( ( char* )Header + ( Rsdt->SdtPointers[ i ] & 0xfff ) );
			}

			if ( _memcmp( Header->Signature, Signature, 4 ) == 0 )
				return Header;

			MmFreeMemory( ( ULONG64 )Header, Header->Length );
		}
	}

	return NULL;
}

VOID
HalInitializeAcpi(

)
{

	Rsdp = NULL;
	for ( ULONG64 i = 0; i < 0x80000; i += 0x10 ) {
		if ( i == 0x20000 ) {

			i = 0x60000;
		}

		if ( _memcmp( ( char* )( 0x80000 + i ), RSDP_SIGNATURE, 8 ) == 0 ) {
			Rsdp = ( ( PRSDP_DESCRIPTOR2_0 )( ( char* )( 0x80000 + i ) ) );
			break;
		}
	}

	if ( Rsdp == NULL ) {

		DbgPrint( L"FATAL: NON-ACPI COMPLIANT SYSTEM, SHUT THE FUCK UP." );
		HalClearInterruptFlag( );
		__halt( );
	}

	if ( Rsdp->Rsdp1_0.Revision == 0 ) {
		//Rsdt = ( PRSDT_DESCRIPTOR )MmpFindFreeVirtual( 0x1000, PAGE_READ | PAGE_WRITE );
		//MmAllocateMemoryAtPhysical( ( ULONG64 )( Rsdp->Rsdp1_0.RsdtAddress&~0xfff ), ( ULONG64 )Rsdt, 0x1000, PAGE_READ | PAGE_WRITE );
		Rsdt = MmAllocateMemoryAtPhysical( ( ULONG64 )( Rsdp->Rsdp1_0.RsdtAddress & ~0xfff ), 0x1000, PAGE_READ | PAGE_WRITE );
		Rsdt = ( PRSDT_DESCRIPTOR )( ( char* )Rsdt + ( Rsdp->Rsdp1_0.RsdtAddress & 0xfff ) );

		if ( Rsdt->Header.Length > ( 0x1000 - Rsdp->Rsdp1_0.RsdtAddress & 0xfff ) ) {
			ULONG32 Length = Rsdt->Header.Length;

			MmFreeMemory( ( ( ULONG64 )Rsdt ) & ~0xfff, 0x1000 );

			//Rsdt = ( PRSDT_DESCRIPTOR )MmpFindFreeVirtual( Length, PAGE_READ | PAGE_WRITE );
			//MmAllocateMemoryAtPhysical( ( ULONG64 )( Rsdp->Rsdp1_0.RsdtAddress&~0xfff ), ( ULONG64 )Rsdt, Length, PAGE_READ | PAGE_WRITE );
			Rsdt = MmAllocateMemoryAtPhysical( ( ULONG64 )( Rsdp->Rsdp1_0.RsdtAddress & ~0xfff ),
				Length + ( Rsdp->Rsdp1_0.RsdtAddress & 0xfff ), PAGE_READ | PAGE_WRITE );
			Rsdt = ( PRSDT_DESCRIPTOR )( ( char* )Rsdt + ( Rsdp->Rsdp1_0.RsdtAddress & 0xfff ) );
		}

		Xsdt = NULL;
	}
	else if ( Rsdp->Rsdp1_0.Revision >= 2 ) {
		//Xsdt = ( PXSDT_DESCRIPTOR )MmpFindFreeVirtual( 0x1000, PAGE_READ | PAGE_WRITE );
		//MmAllocateMemoryAtPhysical( ( ULONG64 )( Rsdp->XsdtAddress&~0xfff ), ( ULONG64 )Xsdt, 0x1000, PAGE_READ | PAGE_WRITE );
		Xsdt = MmAllocateMemoryAtPhysical( ( ULONG64 )( Rsdp->XsdtAddress & ~0xfff ), 0x1000, PAGE_READ | PAGE_WRITE );
		Xsdt = ( PXSDT_DESCRIPTOR )( ( char* )Xsdt + ( Rsdp->XsdtAddress & 0xfff ) );

		if ( Xsdt->Header.Length > ( 0x1000 - ( Rsdp->XsdtAddress & 0xfff ) ) ) {
			ULONG32 Length = Xsdt->Header.Length;

			MmFreeMemory( ( ( ULONG64 )Xsdt )&~0xfff, 0x1000 );

			//Xsdt = ( PXSDT_DESCRIPTOR )MmpFindFreeVirtual( Length, PAGE_READ | PAGE_WRITE );
			//MmAllocateMemoryAtPhysical( ( ULONG64 )( Rsdp->XsdtAddress&~0xfff ), ( ULONG64 )Xsdt, Length, PAGE_READ | PAGE_WRITE );
			Xsdt = MmAllocateMemoryAtPhysical( ( ULONG64 )( Rsdp->XsdtAddress & ~0xfff ),
				Length + ( Rsdp->XsdtAddress & 0xfff ), PAGE_READ | PAGE_WRITE );
			Xsdt = ( PXSDT_DESCRIPTOR )( ( char* )Xsdt + ( Rsdp->XsdtAddress & 0xfff ) );
		}

		Rsdt = NULL;
	}

	Madt = ( PMADT )HalAcpiFindSdt( "APIC" );

	MadtProcessorLocalApics = ExAllocatePoolWithTag( MADT_TABLES_MAX * sizeof( void* ), TAGEX_MPLA );
	MadtIoApics = ExAllocatePoolWithTag( MADT_TABLES_MAX * sizeof( void* ), TAGEX_MIOA );
	MadtInterruptSourceOverrideApics = ExAllocatePoolWithTag( MADT_TABLES_MAX * sizeof( void* ), TAGEX_MISO );
	MadtNonMaskableInterruptApics = ExAllocatePoolWithTag( MADT_TABLES_MAX * sizeof( void* ), TAGEX_MNMI );

	for ( PMADT_HEADER MadtPtr = ( PMADT_HEADER )&Madt->Entry0;
		( ( ULONG64 )MadtPtr ) < ( ( ULONG64 )( ( ( ULONG64 )Madt ) + Madt->Header.Length ) );
		MadtPtr = ( PMADT_HEADER )( ( char* )MadtPtr + MadtPtr->RecordLength ) ) {

		switch ( MadtPtr->EntryType ) {
		case ACPI_MADT_TYPE_PROCESSOR_LOCAL_APIC:
			//DbgPrint("\tMadtProcessorLocalApic\n");
			MadtProcessorLocalApics[ MadtProcessorLocalApicCount++ ] = ( PMADT_PROCESSOR_LOCAL_APIC )MadtPtr;
			break;
		case ACPI_MADT_TYPE_IO_APIC:
			//DbgPrint("\tMadtIoApic\n");
			MadtIoApics[ MadtIoApicCount++ ] = ( PMADT_IO_APIC )MadtPtr;
			break;
		case ACPI_MADT_TYPE_INTERRUPT_SOURCE_OVERRIDE_APIC:
			//DbgPrint("\tMadtInterruptSourceOverrideApic\n");
			MadtInterruptSourceOverrideApics[ MadtInterruptSourceOverrideApicCount++ ] = ( PMADT_INTERRUPT_SOURCE_OVERRIDE_APIC )MadtPtr;
			break;
		case ACPI_MADT_TYPE_NON_MASKABLE_INTERRUPT_APIC:
			//DbgPrint("\tMadtNonMaskableInterruptApic\n");
			MadtNonMaskableInterruptApics[ MadtInterruptSourceOverrideApicCount++ ] = ( PMADT_NON_MASKABLE_INTERRUPT_APIC )MadtPtr;
			break;
		case ACPI_MADT_TYPE_LOCAL_APIC_ADDRESS_OVERRIDE_APIC:
			//DbgPrint("\tMadtLocalApicAddressOverrideApic\n");
			MadtLocalApicAddressOverride = ( PMADT_LOCAL_APIC_ADDRESS_OVERRIDE )MadtPtr;
			break;
		default:
			break;
		}
	}

}

