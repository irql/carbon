


#include <carbsup.h>
#include "mi.h"
#include "ki_struct.h"

KLOCKED_LIST g_AddressSpaceHead = { 0 };
ADDRESS_SPACE_DESCRIPTOR g_KernelPageTable = { 0 };

VOID
MiAllocateAddressSpace(
	__in PADDRESS_SPACE_DESCRIPTOR AddressSpace
)
{

	AddressSpace->BaseVirtual = MmAllocateMemory( 0x1000, PAGE_READ | PAGE_WRITE );
	AddressSpace->BasePhysical = MmPhysicalMapping( ( ULONG64 )AddressSpace->BaseVirtual );

	printf( "addr space alloc: %.16P, %.16P\n", AddressSpace->BasePhysical, AddressSpace->BaseVirtual );

	_memset( AddressSpace->BaseVirtual, 0, 0x800 );
	_memcpy( &AddressSpace->BaseVirtual[ 256 ], &g_KernelPageTable.BaseVirtual[ 256 ], 0x800 );
}

VOID
MiFreeAddressSpace(
	__in PADDRESS_SPACE_DESCRIPTOR AddressSpace
)
{
	AddressSpace;
	//AddressSpace->
}

VOID
MiInsertAddressSpace(
	__in PADDRESS_SPACE_DESCRIPTOR AddressSpace
)
{
	KeAcquireSpinLock( &g_AddressSpaceHead.Lock );

	KeInsertListEntry( g_AddressSpaceHead.List, &AddressSpace->TableLinks );

	KeReleaseSpinLock( &g_AddressSpaceHead.Lock );
}

VOID
MiRemoveAddressSpace(
	__in PADDRESS_SPACE_DESCRIPTOR AddressSpace
)
{
	KeAcquireSpinLock( &g_AddressSpaceHead.Lock );

	KeRemoveListEntry( &AddressSpace->TableLinks );

	KeReleaseSpinLock( &g_AddressSpaceHead.Lock );
}

PADDRESS_SPACE_DESCRIPTOR
MiGetAddressSpace(

)
{

	ULONG64 BasePhysical = __readcr3( );

	if ( BasePhysical == g_KernelPageTable.BasePhysical ) {

		return &g_KernelPageTable;
	}

	MiSetAddressSpace( &g_KernelPageTable );

	KeAcquireSpinLock( &g_AddressSpaceHead.Lock );

	PLIST_ENTRY Flink = g_AddressSpaceHead.List;
	do {
		PADDRESS_SPACE_DESCRIPTOR AddressSpace = CONTAINING_RECORD( Flink, ADDRESS_SPACE_DESCRIPTOR, TableLinks );

		if ( AddressSpace->BasePhysical == BasePhysical ) {

			KeReleaseSpinLock( &g_AddressSpaceHead.Lock );

			MiSetAddressSpace( AddressSpace );
			return AddressSpace;
		}

		Flink = Flink->Flink;
	} while ( Flink != g_AddressSpaceHead.List );

	/* explode. */

	return NULL;
}

VOID
MiSetAddressSpace(
	__in PADDRESS_SPACE_DESCRIPTOR AddressSpace
)
{
	PKPCR Processor = KeQueryCurrentProcessor( );

	if ( Processor != NULL ) {

		if ( Processor->ThreadQueueLength != 0 &&
			Processor->ThreadQueue != NULL ) {

			Processor->ThreadQueue->ThreadControlBlock.AddressSpace = AddressSpace;
		}
	}

	__writecr3( AddressSpace->BasePhysical );
}

VOID
MiEnterKernelSpace(
	__out PADDRESS_SPACE_DESCRIPTOR *PreviousAddressSpace
)
{

	*PreviousAddressSpace = MiGetAddressSpace( );

	MiSetAddressSpace( &g_KernelPageTable );

}

VOID
MiLeaveKernelSpace(
	__in PADDRESS_SPACE_DESCRIPTOR PreviousAddressSpace
)
{

	MiSetAddressSpace( PreviousAddressSpace );
}