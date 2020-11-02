/*++

Module ObjectName:

	bugcheck.c

Abstract:

	Panic.

--*/

#include <carbsup.h>
#include "hal.h"
#include "ki.h"
#include "obp.h"

#include "pesup.h"

VOLATILE ULONG32 KiProcessorShutdown = 0;

EXTERN PVOID KeServiceDescriptorTable[ ];

VOID
printf(
	__in CHAR* Format,
	__in ...
);

VOID
KiProcessorHaltInterrupt(
	__inout PKTRAP_FRAME TrapFrame,
	__in PKPCR Processor
)
{
	Processor;
	TrapFrame;

	printf( "recieved halt ipi on cpu%d\n", Processor->CpuIndex );

	KiProcessorShutdown++;

	HalClearInterruptFlag( );
	HalLocalApicWrite( LOCAL_APIC_LVT_TIMER_REGISTER, LOCAL_APIC_CR0_DEST_DISABLE );
	__halt( );
}

VOID
KiBugCheckTrap(
	__inout PKTRAP_FRAME TrapFrame,
	__in PKPCR Processor
)
{

	static char* Exception[ ] = {
		"div by zero",
		"debug",
		"nmi",
		"breakpoint",
		"overflow",
		"bound range exceeded",
		"invalid opcode",
		"device not available",
		"double fault",
		"???",
		"invalid tss",
		"segment not present",
		"stack segment fault",
		"general protection fault",
		"page fault",
		"???",
		"x87 fpu exception",
		"alignment check",
		"machine check",
		"simd fpu exception",
		"virtualization exception",
		"???",
		"???",
		"???",
		"???",
		"???",
		"???",
		"???",
		"???",
		"security exception"
		"???"
	};

	printf(
		"bugcheck on cpu%d\n"
		"type: %#.2X (%s), err: %#b, tid: %d\n",
		Processor->CpuIndex,
		TrapFrame->Interrupt, Exception[ TrapFrame->Interrupt ], TrapFrame->Error,
		Processor->ThreadQueue->ActiveThreadId
	);

	printf(
		"RAX: %#.16P RBX: %#.16P RCX: %#.16P RDX: %#.16P\n"
		"RSI: %#.16P RDI: %#.16P RBP: %#.16P RSP: %#.16P\n"
		"R8 : %#.16P R9 : %#.16P R10: %#.16P R11: %#.16P\n"
		"R12: %#.16P R13: %#.16P R14: %#.16P R15: %#.16P\n"
		"RIP: %#.16P RFL: %#.8X CS : %#.4X\n"
		"CR0: %#.8X CR2: %#.16P CR3: %#.16P CR4: %#.8X\n",

		TrapFrame->Rax, TrapFrame->Rbx, TrapFrame->Rcx, TrapFrame->Rdx,
		TrapFrame->Rsi, TrapFrame->Rdi, TrapFrame->Rbp, TrapFrame->Rsp,
		TrapFrame->R8, TrapFrame->R9, TrapFrame->R10, TrapFrame->R11,
		TrapFrame->R12, TrapFrame->R13, TrapFrame->R14, TrapFrame->R15,
		TrapFrame->Rip, TrapFrame->Rflags, TrapFrame->CodeSegment,

		__readcr0( ), __readcr2( ), TrapFrame->Cr3, __readcr4( )
	);

	HalIdtInstallHandler( 0xFE, KiProcessorHaltInterrupt );
	for ( ULONG32 i = 0; i < KiLogicalProcessorsInstalled; i++ ) {

		if ( i == Processor->CpuIndex )
			continue;

		PKPCR Pcr;
		KeQueryLogicalProcessor( i, &Pcr );

		HalLocalApicSendIpi( Pcr->AcpiId, LOCAL_APIC_CR0_DEST_NORMAL | 0xFE );

	}

	while ( KiProcessorShutdown != ( KiLogicalProcessorsInstalled - 1 ) )
		;

	printf( "performing ObjectTypeModule dump.\n" );

	PLIST_ENTRY Flink = ObjectTypeModule->ObjectList.List;

	UNICODE_STRING GraphicsDriver = RTL_CONSTANT_UNICODE_STRING( L"\\SystemRoot\\ntgdi.sys" );
	UNICODE_STRING KernelDebugger = RTL_CONSTANT_UNICODE_STRING( L"\\SystemRoot\\kdcom.dll" );
	PVOID KdBase = NULL;

	do {
		POBJECT_ENTRY_HEADER Module = CONTAINING_RECORD( Flink, OBJECT_ENTRY_HEADER, ObjectList );
		PKMODULE ModuleObject = ( PKMODULE )( Module + 1 );

		printf( "%w [%#.16P %#.16P %#.16P]\n", ModuleObject->ImageName.Buffer, ModuleObject->LoaderInfoBlock.ModuleStart, ModuleObject->LoaderInfoBlock.ModuleEnd, ( ULONG64 )ModuleObject->LoaderInfoBlock.ModuleEnd - ( ULONG64 )ModuleObject->LoaderInfoBlock.ModuleStart );

		if ( RtlUnicodeStringCompare( &GraphicsDriver, &ModuleObject->ImageName ) == 0 ) {

			PLIST_ENTRY Flink1 = ObjectTypeDriver->ObjectList.List;

			do {
				POBJECT_ENTRY_HEADER Driver = CONTAINING_RECORD( Flink1, OBJECT_ENTRY_HEADER, ObjectList );
				PDRIVER_OBJECT DriverObject = OB_HEADER2OBJ( Driver );

				if ( DriverObject->DriverModule == ModuleObject ) {

					DriverObject->DriverUnload( DriverObject );
				}

			} while ( Flink1 != ObjectTypeDriver->ObjectList.List );
		}

		if ( RtlUnicodeStringCompare( &KernelDebugger, &ModuleObject->ImageName ) == 0 ) {

			KdBase = ModuleObject->LoaderInfoBlock.ModuleStart;
		}

		Flink = Flink->Flink;
	} while ( Flink != ObjectTypeModule->ObjectList.List );

	if ( KdBase != NULL ) {

		VOID( *KdReportCrash )(
			__inout PKTRAP_FRAME TrapFrame,
			__in PKPCR Processor
			);

		PeSupGetProcedureAddressByName( KdBase, "KdReportCrash", ( PVOID* )&KdReportCrash );
		KdReportCrash( TrapFrame, Processor );
	}

	__halt( );
}


VOID
KiBspBootBugCheckTrap(
	__inout PKTRAP_FRAME TrapFrame,
	__in PKPCR Processor
)
{
	Processor;

	HalClearInterruptFlag( );

#if 0
	printf(
		"KiBspBootBugCheckTrap\nbugcheck on bsp\n"
		"type: %#.2X (%s), err: %#b\n",
		TrapFrame->Interrupt, Exception[ TrapFrame->Interrupt ], TrapFrame->Error
	);

	printf(
		"RAX: %#.16P RBX: %#.16P RCX: %#.16P RDX: %#.16P\n"
		"RSI: %#.16P RDI: %#.16P RBP: %#.16P RSP: %#.16P\n"
		"R8 : %#.16P R9 : %#.16P R10: %#.16P R11: %#.16P\n"
		"R12: %#.16P R13: %#.16P R14: %#.16P R15: %#.16P\n"
		"RIP: %#.16P RFL: %#.8X\n"
		"CR0: %#.8X CR2: %#.16P CR3: %#.16P CR4: %#.8X\n",

		TrapFrame->Rax, TrapFrame->Rbx, TrapFrame->Rcx, TrapFrame->Rdx,
		TrapFrame->Rsi, TrapFrame->Rdi, TrapFrame->Rbp, TrapFrame->Rsp,
		TrapFrame->R8, TrapFrame->R9, TrapFrame->R10, TrapFrame->R11,
		TrapFrame->R12, TrapFrame->R13, TrapFrame->R14, TrapFrame->R15,
		TrapFrame->Rip, TrapFrame->Rflags,

		__readcr0( ), __readcr2( ), __readcr3( ), __readcr4( )
	);
#endif
	EXTERN ULONG64 HalSimdSaveRegion;
	printf( "%d rip:%p, regs:%p %d %p", TrapFrame->Interrupt, TrapFrame->Rip, &TrapFrame->R15, HalSimdSaveRegion, TrapFrame->Rsp );

	__halt( );
}

VOID
KiBspBootBugcheck(
	__in ULONG32 ExceptionCode,
	__in ULONG64 Arg1,
	__in ULONG64 Arg2,
	__in ULONG64 Arg3,
	__in ULONG64 Arg4
)
{
	printf( "KiBspBootBugcheck: %.8X (%P, %P, %P, %P)", ExceptionCode, Arg1, Arg2, Arg3, Arg4 );

	HalClearInterruptFlag( );
	__halt( );
}

VOID
KeBugCheckEx(
	__in ULONG32 ExceptionCode,
	__in ULONG64 Arg1,
	__in ULONG64 Arg2,
	__in ULONG64 Arg3,
	__in ULONG64 Arg4
)
{
	ExceptionCode;
	Arg1;
	Arg2;
	Arg3;
	Arg4;

	KeEnterCriticalRegion( );

	PKPCR Processor = KeQueryCurrentProcessor( );

	HalIdtInstallHandler( 0xFE, KiProcessorHaltInterrupt );
	for ( ULONG32 i = 0; i < KiLogicalProcessorsInstalled; i++ ) {

		if ( i == Processor->CpuIndex ) {

			continue;
		}

		PKPCR CurrentProcessor;
		KeQueryLogicalProcessor( i, &CurrentProcessor );

		HalLocalApicSendIpi( CurrentProcessor->AcpiId, LOCAL_APIC_CR0_DEST_NORMAL | 0xFE );

	}

	while ( KiProcessorShutdown != ( KiLogicalProcessorsInstalled - 1 ) )
		;

	PLIST_ENTRY Flink = ObjectTypeModule->ObjectList.List;

	UNICODE_STRING GraphicsDriver = RTL_CONSTANT_UNICODE_STRING( L"\\SystemRoot\\ntgdi.sys" );

	do {
		POBJECT_ENTRY_HEADER Module = CONTAINING_RECORD( Flink, OBJECT_ENTRY_HEADER, ObjectList );
		PKMODULE ModuleObject = ( PKMODULE )( Module + 1 );

		if ( RtlUnicodeStringCompare( &GraphicsDriver, &ModuleObject->ImageName ) == 0 ) {

			PLIST_ENTRY Flink1 = ObjectTypeDriver->ObjectList.List;

			do {
				POBJECT_ENTRY_HEADER Driver = CONTAINING_RECORD( Flink1, OBJECT_ENTRY_HEADER, ObjectList );
				PDRIVER_OBJECT DriverObject = OB_HEADER2OBJ( Driver );

				if ( DriverObject->DriverModule == ModuleObject ) {

					DriverObject->DriverUnload( DriverObject );
				}

				Flink1 = Flink1->Flink;
			} while ( Flink1 != ObjectTypeDriver->ObjectList.List );
		}

		Flink = Flink->Flink;
	} while ( Flink != ObjectTypeModule->ObjectList.List );

	_memset( VbeGetBasicInfo( )->Framebuffer, 0, VbeGetBasicInfo( )->Height * VbeGetBasicInfo( )->Width * 4 );

	CHAR ExceptionBuffer[ 256 ];

	sprintfA(
		ExceptionBuffer,

		"BUGCHECK CPU%d\n\n"

		"%#.8X (%P, %P, %P, %p)",

		Processor->CpuIndex,

		ExceptionCode, Arg1, Arg2, Arg3, Arg4 );

	HalpDrawString( ExceptionBuffer, 0, 0, 0xFFFFFFFF );

	for ( ULONG32 i = 0; ExceptionBuffer[ i ]; i++ ) {

		( ( USHORT* )0xB8000 )[ i ] = ExceptionBuffer[ i ] | 0xF0;
	}

	ULONG32 y = 16 * 4;

	do {
		POBJECT_ENTRY_HEADER Module = CONTAINING_RECORD( Flink, OBJECT_ENTRY_HEADER, ObjectList );
		PKMODULE ModuleObject = ( PKMODULE )( Module + 1 );

		sprintfA( ExceptionBuffer,
			"%w [%#.16P %#.16P %#.16P]\n",
			ModuleObject->ImageName.Buffer,
			ModuleObject->LoaderInfoBlock.ModuleStart,
			ModuleObject->LoaderInfoBlock.ModuleEnd,
			( ULONG64 )ModuleObject->LoaderInfoBlock.ModuleEnd - ( ULONG64 )ModuleObject->LoaderInfoBlock.ModuleStart );

		HalpDrawString( ExceptionBuffer, 0, y, 0xFFFFFFFF );

		y += 16;


		if ( RtlUnicodeStringCompare( &GraphicsDriver, &ModuleObject->ImageName ) == 0 ) {

			PLIST_ENTRY Flink1 = ObjectTypeDriver->ObjectList.List;

			do {
				POBJECT_ENTRY_HEADER Driver = CONTAINING_RECORD( Flink1, OBJECT_ENTRY_HEADER, ObjectList );
				PDRIVER_OBJECT DriverObject = OB_HEADER2OBJ( Driver );

				if ( DriverObject->DriverModule == ModuleObject ) {

					DriverObject->DriverUnload( DriverObject );
				}

				Flink1 = Flink1->Flink;
			} while ( Flink1 != ObjectTypeDriver->ObjectList.List );
		}

		Flink = Flink->Flink;
	} while ( Flink != ObjectTypeModule->ObjectList.List );

	__halt( );
}
