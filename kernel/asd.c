


#include <carbsup.h>
#include "mm.h"

KLOCKED_LIST AddressSpaceHead = { 0 };
ADDRESS_SPACE_DESCRIPTOR KernelPageTable = { 0 };

VOID
MiAllocateAddressSpace(
	__in PADDRESS_SPACE_DESCRIPTOR AddressSpace
)
{

	//change the shit for page_table_allocation_entries.

	AddressSpace->BaseVirtual = ( ULONG64* )MmAllocateMemory( 0x1000, PAGE_READ | PAGE_WRITE );
	AddressSpace->BasePhysical = ( ULONG64 )MmPhysicalMapping( ( ULONG64 )AddressSpace->BaseVirtual );

	PPAGE_TABLE_ALLOCATION_ENTRY PageTableAlloc = MmAllocateMemory( 0x200000, PAGE_READ | PAGE_WRITE );

	KeInitializeListHead( &PageTableAlloc->AllocationLinks );
	AddressSpace->PageTableAllocationHead = &PageTableAlloc->AllocationLinks;

	PageTableAlloc->AllocationLock.ThreadLocked = 0;
	PageTableAlloc->BaseVirtual = ( ULONG64 )MmAllocateMemory( 0x1000, PAGE_READ | PAGE_WRITE );
	PageTableAlloc->BasePhysical = ( ULONG64 )MmPhysicalMapping( ( ULONG64 )AddressSpace->BaseVirtual );

	_memset( &PageTableAlloc->Bitmap[ 0 ], 0, sizeof( PageTableAlloc->Bitmap ) );
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
	KeAcquireSpinLock( &AddressSpaceHead.Lock );

	KeInsertListEntry( AddressSpaceHead.List, &AddressSpace->TableLinks );

	KeReleaseSpinLock( &AddressSpaceHead.Lock );
}

VOID
MiRemoveAddressSpace(
	__in PADDRESS_SPACE_DESCRIPTOR AddressSpace
)
{
	KeAcquireSpinLock( &AddressSpaceHead.Lock );

	KeRemoveListEntry( &AddressSpace->TableLinks );

	KeReleaseSpinLock( &AddressSpaceHead.Lock );
}

PADDRESS_SPACE_DESCRIPTOR
MiGetAddressSpace(

)
{

	ULONG64 BasePhysical = __readcr3( );

	if ( BasePhysical == KernelPageTable.BasePhysical ) {

		return &KernelPageTable;
	}

	MiSetAddressSpace( &KernelPageTable );

	KeAcquireSpinLock( &AddressSpaceHead.Lock );

	PLIST_ENTRY Flink = AddressSpaceHead.List;
	do {
		PADDRESS_SPACE_DESCRIPTOR AddressSpace = CONTAINING_RECORD( Flink, ADDRESS_SPACE_DESCRIPTOR, TableLinks );

		if ( AddressSpace->BasePhysical == BasePhysical ) {

			KeReleaseSpinLock( &AddressSpaceHead.Lock );

			MiSetAddressSpace( AddressSpace );
			return AddressSpace;
		}

		Flink = Flink->Flink;
	} while ( Flink != AddressSpaceHead.List );

	/* explode. */

	return NULL;
}

VOID
MiSetAddressSpace(
	__in PADDRESS_SPACE_DESCRIPTOR AddressSpace
)
{

	__writecr3( AddressSpace->BasePhysical ); // this reminds me, make the kernel global or you can just page fault here.

}

VOID
MiEnterKernelSpace(
	__out PADDRESS_SPACE_DESCRIPTOR *PreviousAddressSpace
)
{

	*PreviousAddressSpace = MiGetAddressSpace( );

	MiSetAddressSpace( &KernelPageTable );

}

VOID
MiLeaveKernelSpace(
	__in PADDRESS_SPACE_DESCRIPTOR PreviousAddressSpace
)
{

	MiSetAddressSpace( PreviousAddressSpace );
}