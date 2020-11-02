/*++

Module ObjectName:

	thread.c

Abstract:

	Thread management.

--*/

#include <carbsup.h>
#include "hal.h"
#include "ki.h"
#include "obp.h"
#include "mi.h"

EXTERN ADDRESS_SPACE_DESCRIPTOR g_KernelPageTable;

ULONG64 KiThreadDispatcherCounter = 0;

VOID
KiThreadDispatcher(
	__inout PKTRAP_FRAME TrapFrame,
	__in PKPCR Processor
)
{

	KiThreadDispatcherCounter++;

	if ( Processor->ThreadQueue == 0 ) {

		return;
	}

	if ( Processor->ThreadQueueLock.ThreadLocked != 0 ) {
		//queue is locked.

		return;
	}

	HalLocalApicWrite( LOCAL_APIC_LVT_TIMER_REGISTER, HalLocalApicRead( LOCAL_APIC_LVT_TIMER_REGISTER ) | LOCAL_APIC_CR0_DEST_DISABLE );

	if ( Processor->ThreadQueue->ThreadControlBlock.ThreadState != THREAD_STATE_READY &&
		Processor->ThreadQueue->ThreadControlBlock.ThreadState != THREAD_STATE_IDLE ) {

		_memcpy( &Processor->ThreadQueue->ThreadControlBlock.Registers, TrapFrame, sizeof( KTRAP_FRAME ) );

		if ( Processor->ThreadQueue->ThreadControlBlock.ThreadState == THREAD_STATE_RUNNING ) {
			Processor->ThreadQueue->ThreadControlBlock.ThreadState = THREAD_STATE_READY;
		}
	}

	PLIST_ENTRY Flink = Processor->ThreadQueue->ThreadControlBlock.ScheduledThreads.Flink;
	PLIST_ENTRY StartFlink = Processor->ThreadQueue->ThreadControlBlock.ScheduledThreads.Flink;
	do {
		Processor->ThreadQueue = CONTAINING_RECORD( Flink, KTHREAD, ThreadControlBlock.ScheduledThreads.Flink );
		Flink = Flink->Flink;

		if ( Processor->ThreadQueue->ThreadControlBlock.ThreadState == THREAD_STATE_WAITING ) {

			Processor->ThreadQueue->ThreadControlBlock.WaitObject->SwitchProcedure( Processor->ThreadQueue );
		}

		if ( Processor->ThreadQueue->ThreadControlBlock.ThreadState == THREAD_STATE_TERMINATING ) {
			PKTHREAD TerminatingThread = Processor->ThreadQueue;

			Processor->ThreadQueue = CONTAINING_RECORD( Flink, KTHREAD, ThreadControlBlock.ScheduledThreads.Flink );

			TerminatingThread->ThreadControlBlock.ThreadState = THREAD_STATE_TERMINATED;
			KeRemoveListEntry( &TerminatingThread->ThreadControlBlock.ScheduledThreads );
			KeRemoveListEntry( &TerminatingThread->ActiveThreadLinks );
			ObDereferenceObject( TerminatingThread );
		}

	} while (
		Processor->ThreadQueue->ThreadControlBlock.ThreadState != THREAD_STATE_READY &&
		Flink != StartFlink );

	if ( Processor->ThreadQueue->ThreadControlBlock.ThreadState != THREAD_STATE_READY &&
		Processor->ThreadQueue->ThreadControlBlock.ThreadState != THREAD_STATE_IDLE ) {
		Flink = &Processor->ThreadQueue->ThreadControlBlock.ScheduledThreads;
		do {
			Processor->ThreadQueue = CONTAINING_RECORD( Flink, KTHREAD, ThreadControlBlock.ScheduledThreads.Flink );

			Flink = Flink->Flink;
		} while (
			Processor->ThreadQueue->ThreadControlBlock.ThreadState != THREAD_STATE_IDLE );
	}

	if ( Processor->ThreadQueue->ThreadControlBlock.ThreadState != THREAD_STATE_IDLE )
		Processor->ThreadQueue->ThreadControlBlock.ThreadState = THREAD_STATE_RUNNING;

	_memcpy( TrapFrame, &Processor->ThreadQueue->ThreadControlBlock.Registers, sizeof( KTRAP_FRAME ) );
	Processor->TaskState.Rsp0 = Processor->ThreadQueue->KernelStackBase + Processor->ThreadQueue->KernelStackSize;
	Processor->TaskState.Ist3 = Processor->ThreadQueue->ApicStackBase + Processor->ThreadQueue->ApicStackSize;
	Processor->ThreadQueue->ThreadControlBlock.Registers.Cr3 = Processor->ThreadQueue->ThreadControlBlock.AddressSpace->BasePhysical;

	//( ( PGDT_ENTRY_TSS )( ( char* )Processor->Gdtr.Base + Processor->TaskStateDescriptor ) )->AccessByte = 0x89;
	//__ltr( Processor->TaskStateDescriptor | Processor->ThreadQueue->ThreadControlBlock.PrivilegeLevel );

	//( ( PGDT_ENTRY_TSS )( ( char* )Processor->Gdtr.Base + Processor->TaskStateDescriptor ) )->AccessByte = 0x89;
	//__ltr( Processor->TaskStateDescriptor );

	HalLocalApicWrite( LOCAL_APIC_LVT_TIMER_REGISTER, HalLocalApicRead( LOCAL_APIC_LVT_TIMER_REGISTER ) & ~LOCAL_APIC_CR0_DEST_DISABLE );
}

EXTERN ULONG64 KiDispatcherSpinlocks;

VOID
KiInitializeDispatcher(
	__in PKPROCESS KernelProcess
)
{
	PKTHREAD* IdleThread = ExAllocatePoolWithTag( sizeof( PKTHREAD ) * KiLogicalProcessorsInstalled, TAGEX_IDLE );

	for ( ULONG32 i = 0; i < KiLogicalProcessorsInstalled; i++ ) {
		KiCreateThread( &IdleThread[ i ], KernelProcess, ( PKSTART_ROUTINE )KiIdleThread, NULL, 0x1000, 0x1000 );

		if ( i == 0 )
			KeInitializeListHead( &IdleThread[ i ]->ActiveThreadLinks );
		else
			KeInsertListEntry( &IdleThread[ 0 ]->ActiveThreadLinks, &IdleThread[ i ]->ActiveThreadLinks );

		KeInitializeListHead( &IdleThread[ i ]->ThreadControlBlock.ScheduledThreads );
		IdleThread[ i ]->ThreadControlBlock.ThreadState = THREAD_STATE_IDLE;
		IdleThread[ i ]->ThreadControlBlock.LogicalProcessor = i;

		PKPCR Processor;
		KeQueryLogicalProcessor( i, &Processor );

		Processor->ThreadQueue = IdleThread[ i ];
		Processor->ThreadQueueLock.ThreadLocked = 0;
		Processor->ThreadQueueLength = 0;
	}

	return;
}

VOID
KiStartThread(
	__in PKTHREAD Thread
)
{

	ObReferenceObject( Thread );

	PKPCR Processor;
	KeQueryLogicalProcessor( Thread->ThreadControlBlock.LogicalProcessor, &Processor );
	Processor->ThreadQueueLength++;

	KeInsertListEntry( &Processor->ThreadQueue->ThreadControlBlock.ScheduledThreads, &Thread->ThreadControlBlock.ScheduledThreads );
	KeInsertListEntry( &Processor->ThreadQueue->ActiveThreadLinks, &Thread->ActiveThreadLinks );
	Thread->ThreadControlBlock.ThreadState = THREAD_STATE_READY;
}

ULONG32
KiAcquireLowestWorkProcessor(

)
{
	ULONG32 LowestWorkProcessor = 0;
	ULONG32 LowestWork = ( ULONG32 )-1;

	for ( ULONG32 i = 0; i < KiLogicalProcessorsInstalled; i++ ) {

		PKPCR Processor;
		KeQueryLogicalProcessor( i, &Processor );

		if ( LowestWork > Processor->ThreadQueueLength ) {
			LowestWork = Processor->ThreadQueueLength;
			LowestWorkProcessor = i;
		}
	}

	return LowestWorkProcessor;
}

NTSTATUS
KiCreateThread(
	__out PKTHREAD* NewThreadHandle,
	__in PKPROCESS ThreadProcess,
	__in PKSTART_ROUTINE StartRoutine,
	__in PVOID StartContext,
	__in_opt ULONG32 KernelStackSize,
	__in_opt ULONG32 UserStackSize
)
{
	STATIC OBJECT_ATTRIBUTES DefaultAttributes = { 0, NULL };
	NTSTATUS ntStatus;

	PKTHREAD ThreadObject;
	ntStatus = ObpCreateObject( ( PVOID* )&ThreadObject, &DefaultAttributes, ObjectTypeThread );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	ThreadObject->ApicStackSize = 0x4000;
	ThreadObject->KernelStackSize = KernelStackSize == 0 ? 0x4000 : KernelStackSize;
	ThreadObject->UserStackSize = UserStackSize == 0 ? 0x4000 : UserStackSize;

	ThreadObject->ApicStackBase = ( ULONG64 )MmAllocateMemory( ThreadObject->ApicStackSize, PAGE_READ | PAGE_WRITE );
	ThreadObject->KernelStackBase = ( ULONG64 )MmAllocateMemory( ThreadObject->KernelStackSize, PAGE_READ | PAGE_WRITE );
	ThreadObject->UserStackBase = ( ULONG64 )MmAllocateMemory( ThreadObject->UserStackSize, PAGE_READ | PAGE_WRITE );

	ThreadObject->ActiveThreadId = KiGetUniqueIdentifier( );
	ThreadObject->Process = ThreadProcess;
	ObReferenceObject( ThreadProcess );

	ThreadObject->ActiveThreadLinks.Flink = NULL;
	ThreadObject->ActiveThreadLinks.Blink = NULL;

	ThreadObject->ThreadControlBlock.AddressSpace = &g_KernelPageTable;
	ThreadObject->ThreadControlBlock.ScheduledThreads.Flink = NULL;
	ThreadObject->ThreadControlBlock.ScheduledThreads.Blink = NULL;
	ThreadObject->ThreadControlBlock.ThreadState = THREAD_STATE_NOT_READY;

	ThreadObject->ThreadControlBlock.Registers.Rcx = ( ULONG64 )StartContext;

	ThreadObject->ThreadControlBlock.Registers.Rbp = ( ULONG64 )( ( PUCHAR )ThreadObject->UserStackBase + ThreadObject->UserStackSize );
#if 0
	ThreadObject->ThreadControlBlock.Registers.Rsp = ThreadObject->ThreadControlBlock.Registers.Rbp - 40;
#else
	ThreadObject->ThreadControlBlock.Registers.Rsp = ThreadObject->ThreadControlBlock.Registers.Rbp - 48;
	*( ULONG64* )ThreadObject->ThreadControlBlock.Registers.Rsp = ( ULONG64 )KeExitThread; //rsp -> rbp 9/4/2020 - untested?

#endif

	ThreadObject->ThreadControlBlock.Registers.Rflags = 0x200202;

	ThreadObject->ThreadControlBlock.Registers.Rip = ( ULONG64 )StartRoutine;

	ThreadObject->ThreadControlBlock.LogicalProcessor = KiAcquireLowestWorkProcessor( );

	ThreadObject->ThreadControlBlock.Registers.CodeSegment = 8;
	ThreadObject->ThreadControlBlock.Registers.StackSegment = 16;
	ThreadObject->ThreadControlBlock.Registers.DataSegment = 16;
	ThreadObject->ThreadControlBlock.Registers.Cr3 = g_KernelPageTable.BasePhysical;

	ThreadObject->ThreadControlBlock.PrivilegeLevel = 0;

	*NewThreadHandle = ThreadObject;

	return STATUS_SUCCESS;
}

NTSTATUS
KeCreateThread(
	__out PHANDLE ThreadHandle,
	__in HANDLE ProcessHandle,
	__in PKSTART_ROUTINE StartRoutine,
	__in PVOID StartContext,
	__in_opt ULONG32 KernelStackSize,
	__in_opt ULONG32 UserStackSize
)
{
	NTSTATUS ntStatus;

	PKPROCESS ThreadProcess;
	ntStatus = ObReferenceObjectByHandle( ProcessHandle, ( PVOID* )&ThreadProcess );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	PKTHREAD Thread;
	ntStatus = KiCreateThread( &Thread, ThreadProcess, StartRoutine, StartContext, KernelStackSize, UserStackSize );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	ntStatus = ObpCreateHandle( ObpQueryCurrentHandleTable( ), ( PVOID )Thread, ThreadHandle );

	if ( !NT_SUCCESS( ntStatus ) ) {

		//only has one ref.
		ObDereferenceObject( Thread );
		return ntStatus;
	}

	KiStartThread( Thread );

	ObDereferenceObject( Thread );

	return STATUS_SUCCESS;
}

PKTHREAD
KiQueryCurrentThread(

)
{
	PKPCR Processor = KeQueryCurrentProcessor( );

	PKTHREAD Thread = Processor->ThreadQueue;
	ObReferenceObject( Thread );

	return Thread;
}

HANDLE
KeQueryCurrentThread(

)
{
	HANDLE ThreadHandle;
	PKTHREAD Thread = KeQueryCurrentProcessor( )->ThreadQueue;

	ObpCreateHandle( ObpQueryCurrentHandleTable( ), ( PVOID )Thread, &ThreadHandle );

	return ThreadHandle;
}

VOID
KeEnterCriticalRegion(

)
{
	HalClearInterruptFlag( );
	HalLocalApicWrite( LOCAL_APIC_LVT_TIMER_REGISTER, HalLocalApicRead( LOCAL_APIC_LVT_TIMER_REGISTER ) | LOCAL_APIC_CR0_DEST_DISABLE );
}

VOID
KeLeaveCriticalRegion(

)
{

	HalLocalApicWrite( LOCAL_APIC_LVT_TIMER_REGISTER, HalLocalApicRead( LOCAL_APIC_LVT_TIMER_REGISTER ) & ~LOCAL_APIC_CR0_DEST_DISABLE );
	HalSetInterruptFlag( );
}

#if 0
NTSTATUS
KiInsertWaitObject(
	__in HANDLE ThreadHandle,
	__in PWAIT_OBJECT_HEADER WaitObject
)
{
	NTSTATUS ntStatus;
	PKTHREAD ThreadObject;

	ntStatus = ObReferenceObjectByHandle( ThreadHandle, &ThreadObject );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	POBJECT_TYPE_DESCRIPTOR Type;
	ntStatus = ObQueryObjectType( ThreadObject, &Type );

	if ( !NT_SUCCESS( ntStatus ) ) {

		ObDereferenceObject( ThreadObject );
		return ntStatus;
	}

	if ( Type != ObjectTypeThread ) {

		ObDereferenceObject( ThreadObject );
		return STATUS_INVALID_HANDLE;
	}

	ThreadObject->ThreadControlBlock.ThreadState = THREAD_STATE_WAITING;
	ThreadObject->ThreadControlBlock.WaitObject = WaitObject;
}
#endif

VOID
KiDelayExecutionThreadSwitchProcedure(
	__in PKTHREAD ThreadObject
)
{

	PWAIT_OBJECT_DELAY DelayObject = ( PWAIT_OBJECT_DELAY )ThreadObject->ThreadControlBlock.WaitObject;
	if ( ( KiThreadDispatcherCounter - DelayObject->StartTime ) >= DelayObject->SleepTime ) {

		ThreadObject->ThreadControlBlock.ThreadState = THREAD_STATE_READY;
	}

}

NTSTATUS
KeDelayExecutionThread(
	__in ULONG64 Milliseconds
)
{

	Milliseconds /= 10;

	WAIT_OBJECT_DELAY DelayObject;
	DelayObject.SleepTime = Milliseconds;
	DelayObject.StartTime = KiThreadDispatcherCounter;
	DelayObject.Header.SwitchProcedure = KiDelayExecutionThreadSwitchProcedure;

	PKTHREAD ThreadObject = KiQueryCurrentThread( );

	KeEnterCriticalRegion( );
	ThreadObject->ThreadControlBlock.WaitObject = ( PWAIT_OBJECT_HEADER )&DelayObject;
	ThreadObject->ThreadControlBlock.ThreadState = THREAD_STATE_WAITING;
	ObDereferenceObject( ThreadObject );
	KeLeaveCriticalRegion( );
	__halt( );

	return STATUS_SUCCESS;
}

VOID
KiSpinlockWaitThreadSwitchProcedure(
	__in PKTHREAD ThreadObject
)
{

	PWAIT_OBJECT_SPINLOCK SpinObject = ( PWAIT_OBJECT_SPINLOCK )ThreadObject->ThreadControlBlock.WaitObject;
	if ( _InterlockedCompareExchange64( ( volatile long long* )SpinObject->LockQuerying->ThreadLocked, ( long long )ThreadObject, 0 ) == ( long long )ThreadObject ) {

		//SpinObject->LockQuerying->ThreadLocked = ThreadObject;
		ThreadObject->ThreadControlBlock.ThreadState = THREAD_STATE_READY;
	}

}

NTSTATUS
KiSpinlockWaitThread(
	__in PKSPIN_LOCK SpinLock
)
{
	WAIT_OBJECT_SPINLOCK SpinObject;
	SpinObject.LockQuerying = SpinLock;
	SpinObject.Header.SwitchProcedure = KiSpinlockWaitThreadSwitchProcedure;

	PKTHREAD ThreadObject = KiQueryCurrentThread( );

	KeEnterCriticalRegion( );
	ThreadObject->ThreadControlBlock.WaitObject = ( PWAIT_OBJECT_HEADER )&SpinObject;
	ThreadObject->ThreadControlBlock.ThreadState = THREAD_STATE_WAITING;
	ObDereferenceObject( ThreadObject );
	KeLeaveCriticalRegion( );
	__halt( );

	return STATUS_SUCCESS;
}

NTSTATUS
KeTerminateThread(
	__in HANDLE ThreadHandle
)
{

	NTSTATUS ntStatus;
	PKTHREAD ThreadObject;

	if ( ThreadHandle == ( HANDLE )-1 ) {

		ThreadObject = KiQueryCurrentThread( );
	}
	else {

		ntStatus = ObReferenceObjectByHandle( ThreadHandle, &ThreadObject );

		if ( !NT_SUCCESS( ntStatus ) ) {

			return ntStatus;
		}

		POBJECT_TYPE_DESCRIPTOR Type;
		ntStatus = ObQueryObjectType( ThreadObject, &Type );

		if ( !NT_SUCCESS( ntStatus ) ) {

			ObDereferenceObject( ThreadObject );
			return ntStatus;
		}

		if ( Type != ObjectTypeThread ) {

			ObDereferenceObject( ThreadObject );
			return STATUS_INVALID_HANDLE;
		}
	}

	KeEnterCriticalRegion( );
	ThreadObject->ThreadControlBlock.ThreadState = THREAD_STATE_TERMINATING;
	ThreadObject->Process->ActiveThreads--;


	PKTHREAD CurrentThread = KiQueryCurrentThread( );
	ObDereferenceObject( CurrentThread );
	ObDereferenceObject( ThreadObject );
	KeLeaveCriticalRegion( );

	if ( ThreadObject == CurrentThread )
		__halt( );

	return STATUS_SUCCESS;
}

VOID
KeExitThread(

)
{
	/*
		Function only really exists so i can terminate threads on return.
	*/
	KeTerminateThread( ( HANDLE )-1 );
}