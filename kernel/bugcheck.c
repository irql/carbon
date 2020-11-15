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
#include "rtlp.h"

#include "pesup.h"

VOLATILE ULONG32 KiProcessorShutdown = 0;

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
		TrapFrame->Rip, TrapFrame->EFlags,

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

	EXTERN ULONG printf_x, printf_y;
	//_memset( VbeGetBasicInfo( )->Framebuffer, 0, VbeGetBasicInfo( )->Height * VbeGetBasicInfo( )->Width * 4 );

	for ( ULONG32 i = 0; i < VbeGetBasicInfo( )->Height * VbeGetBasicInfo( )->Width; i++ ) {

		VbeGetBasicInfo( )->Framebuffer[ i ] = 0xFF24292E;
	}

	printf_x = 0;
	printf_y = 0;

	PKTRAP_FRAME TrapFrame = ( PKTRAP_FRAME )Arg3;
	CONTEXT TargetContext;

	RtlTrapFrameToContext( TrapFrame, &TargetContext );

	printf(
		"BUGCHECK CPU%d\n\n"

		"%#.8X (%P, %P, %P, %P)\n\n",

		Processor->CpuIndex,

		ExceptionCode, Arg1, Arg2, Arg3, Arg4 );

	printf(
		"RAX: %#.16P RBX: %#.16P RCX: %#.16P RDX: %#.16P\n"
		"RSI: %#.16P RDI: %#.16P RBP: %#.16P RSP: %#.16P\n"
		"R8 : %#.16P R9 : %#.16P R10: %#.16P R11: %#.16P\n"
		"R12: %#.16P R13: %#.16P R14: %#.16P R15: %#.16P\n"
		"RIP: %#.16P RFL: %#.8X CS : %#.4X\n"
		"CR3: %#.16P\n\n",

		TrapFrame->Rax, TrapFrame->Rbx, TrapFrame->Rcx, TrapFrame->Rdx,
		TrapFrame->Rsi, TrapFrame->Rdi, TrapFrame->Rbp, TrapFrame->Rsp,
		TrapFrame->R8, TrapFrame->R9, TrapFrame->R10, TrapFrame->R11,
		TrapFrame->R12, TrapFrame->R13, TrapFrame->R14, TrapFrame->R15,
		TrapFrame->Rip, TrapFrame->EFlags, TrapFrame->CodeSegment,

		TrapFrame->Cr3
	);

	PVAD ExceptionVad = RtlpFindTargetModule( Processor->ThreadQueue, &TargetContext );

	printf( "Fault thread : 0x%.4x\nFault process: 0x%.4x\nFault module : %w\nProcess name : %w\n\n",
		Processor->ThreadQueue->ActiveThreadId,
		Processor->ThreadQueue->Process->ActiveProcessId,
        ExceptionVad ? ExceptionVad->RangeName.Buffer : NULL,
		Processor->ThreadQueue->Process->VadTree.RangeName.Buffer );

	printf( "Unwinding call stack...\nAddress          Frame\n%.16P %.16P %w!%.8X\n", 
		TargetContext.Rip, 
		TargetContext.Rsp, 
        ExceptionVad ? ExceptionVad->RangeName.Buffer : NULL,
		TargetContext.Rip - (ExceptionVad ? ( ULONG64 )ExceptionVad->Range.ModuleStart : 0) );

	while ( NT_SUCCESS( RtlUnwind( ( PKTHREAD )Arg4, &TargetContext ) ) ) {

		ExceptionVad = RtlpFindTargetModule( Processor->ThreadQueue, &TargetContext );

		printf( "%.16P %.16P %w!%.8X\n", 
			TargetContext.Rip, 
			TargetContext.Rsp, 
            ExceptionVad ? ExceptionVad->RangeName.Buffer : NULL,
            TargetContext.Rip - ( ExceptionVad ? ( ULONG64 )ExceptionVad->Range.ModuleStart : 0 ) );

		//removed limit checks for syscall fix.

	}

	printf( "\n" );

	do {
		POBJECT_ENTRY_HEADER Module = CONTAINING_RECORD( Flink, OBJECT_ENTRY_HEADER, ObjectList );
		PKMODULE ModuleObject = ( PKMODULE )( Module + 1 );

		printf( "%w [%.16P %.16P %.16P]\n",
			ModuleObject->ImageName.Buffer,
			ModuleObject->LoaderInfoBlock.ModuleStart,
			ModuleObject->LoaderInfoBlock.ModuleEnd,
			( ULONG64 )ModuleObject->LoaderInfoBlock.ModuleEnd - ( ULONG64 )ModuleObject->LoaderInfoBlock.ModuleStart );


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
