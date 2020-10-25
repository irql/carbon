/*++

Module ObjectName:

	initos.c

Abstract:

	Defines the system initialization routines.

--*/

#include <carbsup.h>
#include "hal.h"
#include "acpi.h"
#include "mm.h"
#include "ldrsup.h"
#include "ob.h"
#include "ex.h"
#include "ke.h"
#include "io.h"

EXTERN VOLATILE ULONG32 KiCurrentCpuInitializing;
EXTERN ULONG32 KiDispatcherSpinlocks;

VOID
KiSystemThread(
	__in PVOID DiskDriverFile
)
{

	KiDispatcherSpinlocks = 1;

	MmFreeMemory( 0x1000, 0xF000 );
	MiMarkPhysical( 0x1000, 15, TRUE );

	NTSTATUS ntStatus;
	PKMODULE DiskModule;
	OBJECT_ATTRIBUTES DefaultAttributes = { 0, NULL };

	ntStatus = ObpCreateObject( &DiskModule, &DefaultAttributes, ObjectTypeModule );

	if ( !NT_SUCCESS( ntStatus ) ) {

		HalClearInterruptFlag( );
		__halt( );
	}

	ntStatus = LdrpSupLoadModule( DiskDriverFile, &DiskModule->LoaderInfoBlock );

	if ( !NT_SUCCESS( ntStatus ) ) {

		HalClearInterruptFlag( );
		__halt( );
	}

	UNICODE_STRING DiskFilePath = RTL_CONSTANT_UNICODE_STRING( L"\\SystemRoot\\disk.sys" );
	_memcpy( ( void* )&DiskModule->ImageName, &DiskFilePath, sizeof( UNICODE_STRING ) );

	PDRIVER_OBJECT DiskDriver;
	IopCreateDriver( &DiskDriver, DiskModule, &DiskFilePath );

	( ( PKDRIVER_LOAD )DiskModule->LoaderInfoBlock.ModuleEntry )( DiskDriver );

	UNICODE_STRING FilePath = RTL_CONSTANT_UNICODE_STRING( L"\\SystemRoot\\serial.sys" );
	NTSTATUS Load = IoLoadDriver( &FilePath );

	printf( "%x\n", Load );

	UNICODE_STRING FilePath2 = RTL_CONSTANT_UNICODE_STRING( L"\\SystemRoot\\kdcom.dll" );
	Load = LdrLoadDll( &FilePath2 );

	printf( "%x\n", Load );

	UNICODE_STRING FilePath1 = RTL_CONSTANT_UNICODE_STRING( L"\\SystemRoot\\ntgdi.sys" );
	Load = IoLoadDriver( &FilePath1 );

	printf( "%x\n", Load );

	while ( 1 )
		__halt( );
}

VOID
VbeSetDisplay(
	__in PVBE_INFO Info
);

INTERRUPT_DESCRIPTOR_TABLE BspBootDescriptor[ 256 ];

VOID
KiBspBootBugCheckTrap(
	__inout PKTRAP_FRAME TrapFrame,
	__in PKPCR Processor
);

VOID
KiSystemStartup(
	__in PE820MM MemoryMap,
	__in ULONG64 DiskDriverFile,
	__in PVBE_INFO VbeInfo
)
/*++

Routine Description:

	Abc.

Arguments:

	MemoryMap - Supplies the physical memory map from the bootloader.

	DiskDriverFile - supplies a pointer to the base of the disk driver file.
		(\system\disk.sys)

Return Value:

	None.

--*/

{
	/* SSE2 is standard for x86_64 and shit will cum if we dont enable it. */
	HalSmpEnableCpuFeatures( );

	HalPic8259aSetIrqMasks( 0xff, 0xff );
	HalPic8259aRemapVectorOffsets( 32, 40 );

	EXTERN ULONG64 Framebuffer;
	EXTERN USHORT XResolution;
	EXTERN USHORT YResolution;
	XResolution = VbeInfo->Width;
	YResolution = VbeInfo->Height;
	Framebuffer = 0xA0000;


	IDTR BspIdtr;
	for ( USHORT i = 0; i < 256; i++ ) {

		BspBootDescriptor[ i ].Offset0 = ( USHORT )( HalInterruptHandlerTable[ i ] );
		BspBootDescriptor[ i ].Offset1 = ( USHORT )( HalInterruptHandlerTable[ i ] >> 16 );
		BspBootDescriptor[ i ].Offset2 = ( ULONG32 )( HalInterruptHandlerTable[ i ] >> 32 );
		BspBootDescriptor[ i ].InterruptStackTable = 0;
		BspBootDescriptor[ i ].Zero = 0;
		BspBootDescriptor[ i ].CodeSelector = 8;
		BspBootDescriptor[ i ].TypeAttribute = 0x80 | ( i < 32 ? IDT_GATE_TYPE_TRAP32 : IDT_GATE_TYPE_INTERRUPT32 );
	}

	BspIdtr.Base = ( ULONG64 )BspBootDescriptor;
	BspIdtr.Limit = 256 * sizeof( INTERRUPT_DESCRIPTOR_TABLE ) - 1;

	for ( UCHAR i = 0; i < 32; i++ )
		HalIdtInstallHandler( i, KiBspBootBugCheckTrap );
	__lidt( &BspIdtr );
	HalSetInterruptFlag( );

	ULONG64 TotalInstalledMemory = MiInitMemoryManager( MemoryMap );
	TotalInstalledMemory;

	/* should be done in the boot loader really. */
	DiskDriverFile = ( DiskDriverFile >> 12 ) + ( DiskDriverFile & 0xFFFF );

	VbeSetDisplay( VbeInfo );

	ExpInitializePoolAllocation( );
	ObpInitializeObjectManager( );

	HalPciEnumerate( );
	HalInitializeAcpi( );
	HalLocalApicEnable( );

	HalSmpInitializeCpu0( );

	OBJECT_ATTRIBUTES ObjectAttributes = { OBJ_PERMANENT, NULL };
	UNICODE_STRING KernelName = RTL_CONSTANT_UNICODE_STRING( L"\\SystemRoot\\kernel.sys" );

	ObjectAttributes.ObjectName = &KernelName;
	PKMODULE KernelModule;
	ObpCreateObject( ( PVOID* )&KernelModule, &ObjectAttributes, ObjectTypeModule );

	LdrpSupGetInfoBlock( ( PVOID )KERNEL_IMAGE_BASE, &KernelModule->LoaderInfoBlock );
	_memcpy( &KernelModule->ImageName, &KernelName, sizeof( UNICODE_STRING ) );

	UNICODE_STRING SystemRootDirectory = RTL_CONSTANT_UNICODE_STRING( L"\\Harddisk\\PhysicalDrive0\\Partition0\\system" );
	UNICODE_STRING SystemRoot = RTL_CONSTANT_UNICODE_STRING( L"\\SystemRoot" );
	ObpCreateSymbolicLink( &SystemRoot, NULL, &SystemRootDirectory );

	UNICODE_STRING RootDirectory = RTL_CONSTANT_UNICODE_STRING( L"\\Harddisk\\PhysicalDrive0\\Partition0" );
	UNICODE_STRING Drive0 = RTL_CONSTANT_UNICODE_STRING( L"C:" );
	ObpCreateSymbolicLink( &Drive0, NULL, &RootDirectory );

	//Identity mapped.
	UCHAR* Buffer = ( UCHAR* )0x1000;
	_memcpy( Buffer, ( void* )HalSmpTrampolineStart, 0x200 );
	*( ULONG32* )0x1600 = ( ULONG32 )__readcr3( );
	*( ULONG64* )0x1808 = ( ULONG64 )HalSmpInitializeCpu;

	for ( ULONG32 i = 1, j = 0; i < MadtProcessorLocalApicCount; i++, j = 0 ) {

		*( ULONG64* )0x1800 = ( ULONG64 )MmAllocateMemory( 0x1000, PAGE_READ | PAGE_WRITE );

		KiCurrentCpuInitializing = MadtProcessorLocalApics[ i ]->ApicId;

		HalLocalApicSendIpi( MadtProcessorLocalApics[ i ]->ApicId, LOCAL_APIC_CR0_DEST_INIT_OR_INIT_DEASSERT | LOCAL_APIC_CR0_NO_INIT_DEASSERT );

		HalPitDelay( 10 );

		HalLocalApicSendIpi( MadtProcessorLocalApics[ i ]->ApicId, LOCAL_APIC_CR0_NO_INIT_DEASSERT | LOCAL_APIC_CR0_DEST_SIPI | 1 );

		while ( j != 10 && KiCurrentCpuInitializing != 0 ) {
			HalPitDelay( 1 );
			j++;
		}

		if ( KiCurrentCpuInitializing != 0 ) {

			HalLocalApicSendIpi( MadtProcessorLocalApics[ i ]->ApicId, LOCAL_APIC_CR0_NO_INIT_DEASSERT | LOCAL_APIC_CR0_DEST_SIPI | 1 );

			j = 0;

			while ( j < 1000 && KiCurrentCpuInitializing != 0 ) {
				HalPitDelay( 1 );
				j++;
			}

			if ( KiCurrentCpuInitializing != 0 ) {

				continue;
			}
		}
	}

	MmFreeMemory( 0, 0x1000 );

	PKPROCESS KernelProcess;
	KiCreateProcess( &KernelProcess, KernelModule, &KernelName );

	PKTHREAD KernelThread;
	KiInitializeDispatcher( KernelProcess );

	KiCreateThread( &KernelThread, KernelProcess, ( PKSTART_ROUTINE )KiSystemThread, ( PVOID )DiskDriverFile, 0x4000, 0x4000 );
	KiStartThread( KernelThread );

	ObDereferenceObject( KernelThread );
	HalLocalApicTimerEnable( );

	while ( 1 )
		__halt( );
}

