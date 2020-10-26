/*++

Module ObjectName:

	processor.c

Abstract:

	Processor management.

--*/

#include <carbsup.h>
#include "hal.h"
#include "ke.h"
#include "acpi.h"

PKPCR* KiLogicalProcessors = 0;
ULONG32 KiLogicalProcessorsInstalled = 0;

NTSTATUS
KiCreateKpcr(
	__in ULONG32 AcpiId,
	__out PKPCR* Kpcr
)
{

	if ( KiLogicalProcessors == 0 ) {

		KiLogicalProcessors = ( PKPCR* )ExAllocatePoolWithTag( sizeof( PKPCR ) * MadtProcessorLocalApicCount, TAGEX_CPU );

		if ( KiLogicalProcessors == NULL ) {

			return STATUS_UNSUCCESSFUL;
		}
	}

	*Kpcr = ( PKPCR )ExAllocatePoolWithTag( sizeof( KPCR ), TAGEX_KPCR );

	if ( *Kpcr == NULL ) {

		return STATUS_UNSUCCESSFUL;
	}

	_memset( ( void* )( *Kpcr ), 0, sizeof( KPCR ) );

	( *Kpcr )->AcpiId = AcpiId;
	( *Kpcr )->CpuIndex = KiLogicalProcessorsInstalled;
	( *Kpcr )->ThreadQueue = 0;
	( *Kpcr )->ThreadQueueLock.ThreadLocked = 0;

	KiLogicalProcessors[ KiLogicalProcessorsInstalled++ ] = *Kpcr;

	return STATUS_SUCCESS;
}

NTSTATUS
KeQueryLogicalProcessor(
	__in ULONG32 ProcessorIndex,
	__in PKPCR* Kpcr
)
{
	if ( ProcessorIndex > KiLogicalProcessorsInstalled ) {

		return STATUS_UNSUCCESSFUL;
	}

	*Kpcr = KiLogicalProcessors[ ProcessorIndex ];

	return STATUS_SUCCESS;
}

PKPCR
KeQueryCurrentProcessor(

)
{

	return ( PKPCR )__readmsr( MSR_GS_KERNEL_BASE );
}

ULONG32
KeQueryProcessorCount(

)
{

	return KiLogicalProcessorsInstalled;
}