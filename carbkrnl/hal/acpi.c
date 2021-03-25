


#include <carbsup.h>
#include "halp.h"

PSDT_HEADER
HalAcpiFindSdt(
    _In_ PCHAR Signature
)
{
    PSDT_HEADER Header;
    ULONG32 Entries;
    ULONG32 Length;
    ULONG32 i;

    if ( HalXsdt != NULL ) {
        Entries = ( HalXsdt->Header.Length - sizeof( SDT_HEADER ) ) / 8;

        for ( i = 0; i < Entries; i++ ) {

            Header = MmMapIoSpace( HalXsdt->SdtPointers[ i ], 0x1000 );

            if ( Header->Length > 0x1000 ) {
                Length = Header->Length;

                MmUnmapIoSpace( Header );

                Header = MmMapIoSpace( HalXsdt->SdtPointers[ i ], Length );
            }

            if ( RtlCompareMemory( Header->Signature, Signature, 4 ) == 0 ) {
                return Header;
            }

            MmUnmapIoSpace( Header );
        }
    }
    else {
        Entries = ( HalRsdt->Header.Length - sizeof( SDT_HEADER ) ) / 8;

        for ( i = 0; i < Entries; i++ ) {

            Header = MmMapIoSpace( HalRsdt->SdtPointers[ i ], 0x1000 );

            if ( Header->Length > 0x1000 ) {
                Length = Header->Length;

                MmUnmapIoSpace( Header );

                Header = MmMapIoSpace( HalRsdt->SdtPointers[ i ], Length );
            }

            if ( RtlCompareMemory( Header->Signature, Signature, 4 ) == 0 ) {
                return Header;
            }

            MmUnmapIoSpace( Header );
        }
    }

    return NULL;
}

VOID
HalInitializeAcpi(

)
{
    PMADT_HEADER MadtPtr;
    ULONG64 i;
    ULONG32 Length;

    for ( i = 0; i < 0x80000; i += 0x10 ) {

        if ( i == 0x20000 ) {
            i = 0x60000;
        }

        if ( RtlCompareMemory( ( char* )( 0x80000 + i ), "RSD PTR ", 8 ) == 0 ) {
            HalRsdp = ( PRSDP_DESCRIPTOR2_0 )( 0x80000 + i );
            break;
        }
    }

    if ( HalRsdp == NULL ) {

        __writecr2( 'IPCA' );
        __halt( );
    }

    if ( HalRsdp->Rsdp1_0.Revision == 0 ) {
        HalRsdt = MmMapIoSpace( HalRsdp->Rsdp1_0.RsdtAddress, 0x1000 );

        if ( HalRsdt->Header.Length > 0x1000 ) {
            Length = HalRsdt->Header.Length;

            MmUnmapIoSpace( HalRsdt );

            HalRsdt = MmMapIoSpace( HalRsdp->Rsdp1_0.RsdtAddress, Length );
        }
        HalXsdt = NULL;
    }
    else if ( HalRsdp->Rsdp1_0.Revision >= 2 ) {
        HalXsdt = MmMapIoSpace( HalRsdp->XsdtAddress, 0x1000 );

        if ( HalXsdt->Header.Length > 0x1000 ) {
            Length = HalXsdt->Header.Length;

            MmUnmapIoSpace( HalXsdt );

            HalXsdt = MmMapIoSpace( HalRsdp->XsdtAddress, Length );
        }
        HalRsdt = NULL;
    }

    HalMadt = ( PMADT )HalAcpiFindSdt( "APIC" );

    HalLocalApics = MmAllocatePoolWithTag( NonPagedPool, MADT_TABLES_MAX * sizeof( PVOID ), 'TDAM' );
    HalIoApics = MmAllocatePoolWithTag( NonPagedPool, MADT_TABLES_MAX * sizeof( PVOID ), 'TDAM' );
    HalIsoApics = MmAllocatePoolWithTag( NonPagedPool, MADT_TABLES_MAX * sizeof( PVOID ), 'TDAM' );
    HalNmiApics = MmAllocatePoolWithTag( NonPagedPool, MADT_TABLES_MAX * sizeof( PVOID ), 'TDAM' );

    for ( MadtPtr = ( PMADT_HEADER )&HalMadt->Entry0;
        ( ( ULONG64 )MadtPtr ) < ( ( ULONG64 )( ( ( ULONG64 )HalMadt ) + HalMadt->Header.Length ) );
          MadtPtr = ( PMADT_HEADER )( ( PUCHAR )MadtPtr + MadtPtr->RecordLength ) ) {
        switch ( MadtPtr->EntryType ) {
        case ACPI_MADT_TYPE_PROCESSOR_LOCAL_APIC:
            HalLocalApics[ HalLocalApicCount++ ] = ( PMADT_PROCESSOR_LOCAL_APIC )MadtPtr;
            break;
        case ACPI_MADT_TYPE_IO_APIC:
            HalIoApics[ HalIoApicCount++ ] = ( PMADT_IO_APIC )MadtPtr;
            break;
        case ACPI_MADT_TYPE_INTERRUPT_SOURCE_OVERRIDE_APIC:
            HalIsoApics[ HalIsoApicCount++ ] = ( PMADT_INTERRUPT_SOURCE_OVERRIDE_APIC )MadtPtr;
            break;
        case ACPI_MADT_TYPE_NON_MASKABLE_INTERRUPT_APIC:
            HalNmiApics[ HalNmiApicCount++ ] = ( PMADT_NON_MASKABLE_INTERRUPT_APIC )MadtPtr;
            break;
        case ACPI_MADT_TYPE_LOCAL_APIC_ADDRESS_OVERRIDE_APIC:
            HalLocalApicAddressOverride = ( PMADT_LOCAL_APIC_ADDRESS_OVERRIDE )MadtPtr;
            break;
        default:
            break;
        }
    }

}
