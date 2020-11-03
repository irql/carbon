


#include <carbsup.h>
#include "psp.h"
#include "ki.h"
#include "mi.h"
#include "ldrp.h"
#include "ldrpsup.h"
#include "ldrpusr.h"

NTSTATUS
PsCreateUserProcess(
	__out PHANDLE         ProcessHandle,
	__in  PUNICODE_STRING FileName
)
{
	FileName;

	STATIC OBJECT_ATTRIBUTES DefaultAttributes = { OBJ_PERMANENT, NULL };

	NTSTATUS ntStatus;
	PKPROCESS ProcessObject;
	PADDRESS_SPACE_DESCRIPTOR PreviousAddressSpace;

	ntStatus = ObpCreateObject( &ProcessObject, &DefaultAttributes, ObjectTypeProcess );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	MiAllocateAddressSpace( &ProcessObject->AddressSpace );
	MiInsertAddressSpace( &ProcessObject->AddressSpace );

	PreviousAddressSpace = MiGetAddressSpace( );
	MiSetAddressSpace( &ProcessObject->AddressSpace );

	ntStatus = LdrpUsrLoadModule( ProcessObject, FileName );

	if ( !NT_SUCCESS( ntStatus ) ) {

		MiSetAddressSpace( PreviousAddressSpace );
		MiRemoveAddressSpace( &ProcessObject->AddressSpace );
		MiFreeAddressSpace( &ProcessObject->AddressSpace );
		ObDestroyObject( ProcessObject );
		return ntStatus;
	}

	MiSetAddressSpace( PreviousAddressSpace );

	ntStatus = ObCreateHandle( ProcessHandle, ProcessObject );

	if ( !NT_SUCCESS( ntStatus ) ) {

		MiRemoveAddressSpace( &ProcessObject->AddressSpace );
		MiFreeAddressSpace( &ProcessObject->AddressSpace );
		ObDestroyObject( ProcessObject );
		return ntStatus;
	}

	HANDLE FirstThread;
	ntStatus = PsCreateUserThread(
		&FirstThread,
		*ProcessHandle,
		( PKSTART_ROUTINE )ProcessObject->VadTree.Range.ModuleEntry,
		NULL,
		0,
		0x4000
	);

	if ( !NT_SUCCESS( ntStatus ) ) {

		MiRemoveAddressSpace( &ProcessObject->AddressSpace );
		MiFreeAddressSpace( &ProcessObject->AddressSpace );
		ObDestroyObject( ProcessObject );
		return ntStatus;
	}

	PspInsertProcess( ProcessObject );
	ObDereferenceObject( ProcessObject );

	return STATUS_SUCCESS;
}

NTSTATUS
PsCreateUserThread(
	__out    PHANDLE         ThreadHandle,
	__in     HANDLE          ProcessHandle,
	__in     PKSTART_ROUTINE ThreadStart,
	__in     PVOID           ThreadContext,
	__in     ULONG32         ThreadFlags,
	__in_opt ULONG32         UserStackSize
)
{
	ThreadHandle;
	ProcessHandle;
	ThreadStart;
	ThreadContext;
	ThreadFlags;
	UserStackSize;

	//
	// implement some ThreadFlags, for starting a thread suspended or other shit i guess.
	// also make a kernel more equivalent and adjust the Ke routines for thread/process creation
	// these routines should idealy be removed and Psapi should take over.
	//
	// also fix the object managers reference problems and rewrite some table.c & asd.c code
	// address space descriptors are useless if you're going to cpu global the kernel's memory.
	//

	STATIC OBJECT_ATTRIBUTES DefaultAttributes = { 0, NULL };
	NTSTATUS ntStatus;

	PKPROCESS ProcessObject;

	ntStatus = ObReferenceObjectByHandle( ProcessHandle, &ProcessObject );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	PKTHREAD ThreadObject;
	ntStatus = ObpCreateObject( &ThreadObject, &DefaultAttributes, ObjectTypeThread );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	ntStatus = ObCreateHandle( ThreadHandle, ThreadObject );

	if ( !NT_SUCCESS( ntStatus ) ) {

		ObDestroyObject( ThreadObject );
		return ntStatus;
	}

	ThreadObject->ApicStackSize = 0x4000;
	ThreadObject->KernelStackSize = 0x4000;
	ThreadObject->UserStackSize = UserStackSize == 0 ? 0x4000 : UserStackSize;

	PADDRESS_SPACE_DESCRIPTOR PreviousAddressSpace = MiGetAddressSpace( );

	MiSetAddressSpace( &ProcessObject->AddressSpace );
	ThreadObject->ApicStackBase = ( ULONG64 )MmAllocateMemory( ThreadObject->ApicStackSize, PAGE_READ | PAGE_WRITE );
	ThreadObject->KernelStackBase = ( ULONG64 )MmAllocateMemory( ThreadObject->KernelStackSize, PAGE_READ | PAGE_WRITE );
	ThreadObject->UserStackBase = ( ULONG64 )MmAllocateMemory( ThreadObject->UserStackSize, PAGE_READ | PAGE_WRITE | PAGE_USER );
	MiSetAddressSpace( PreviousAddressSpace );

	ThreadObject->ActiveThreadId = KiGetUniqueIdentifier( );
	ThreadObject->Process = ProcessObject;
	ObReferenceObject( ProcessObject );

	ThreadObject->ActiveThreadLinks.Flink = NULL;
	ThreadObject->ActiveThreadLinks.Blink = NULL;

	ThreadObject->ThreadControlBlock.AddressSpace = &ProcessObject->AddressSpace;
	ThreadObject->ThreadControlBlock.ScheduledThreads.Flink = NULL;
	ThreadObject->ThreadControlBlock.ScheduledThreads.Blink = NULL;
	ThreadObject->ThreadControlBlock.ThreadState = 0;
	ThreadObject->ThreadControlBlock.ThreadExitCode = 0;

	ThreadObject->ThreadControlBlock.Registers.Rcx = ( ULONG64 )ThreadContext;
	ThreadObject->ThreadControlBlock.Registers.Rbp = ThreadObject->UserStackBase + ( ULONG64 )ThreadObject->UserStackSize;
	ThreadObject->ThreadControlBlock.Registers.Rsp = ThreadObject->ThreadControlBlock.Registers.Rbp - 48;
	*( ULONG64* )ThreadObject->ThreadControlBlock.Registers.Rsp = ( ULONG64 )KeExitThread; // write ntdll.dll

	ThreadObject->ThreadControlBlock.Registers.EFlags = 0x203202;
	ThreadObject->ThreadControlBlock.Registers.Rip = ( ULONG64 )ThreadStart;
	ThreadObject->ThreadControlBlock.LogicalProcessor = KiAcquireLowestWorkProcessor( );

	ThreadObject->ThreadControlBlock.Registers.CodeSegment = GDT_USER_CODE64 | 0x3;
	ThreadObject->ThreadControlBlock.Registers.DataSegment = GDT_USER_DATA | 0x3;
	ThreadObject->ThreadControlBlock.Registers.StackSegment = GDT_USER_DATA | 0x3;
	ThreadObject->ThreadControlBlock.Registers.Cr3 = ProcessObject->AddressSpace.BasePhysical;

	ThreadObject->ThreadControlBlock.PrivilegeLevel = 3;

	printf( "new thread with id: %d\n", ThreadObject->ActiveThreadId );
	KiStartThread( ThreadObject );

	return STATUS_SUCCESS;
}
