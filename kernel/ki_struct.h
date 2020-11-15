


#pragma once

#define THREAD_STATE_IDLE			0x00
#define THREAD_STATE_RUNNING		0x01
#define THREAD_STATE_READY			0x02
#define THREAD_STATE_NOT_READY		0x04
#define THREAD_STATE_TERMINATING	0x08
#define THREAD_STATE_TERMINATED		0x10
#define THREAD_STATE_WAITING		0x20

#include "mi.h"

typedef struct _VAD {
    LDR_INFO_BLOCK Range;
    UNICODE_STRING RangeName;

    struct _VAD*   Next;
} VAD, *PVAD;

#if 0
typedef struct _KDEBUGGER {


} KDEBUGGER, *PKDEBUGGER;
#endif //impl later you greaseball.

typedef struct _KPROCESS {
	ULONG32 ActiveProcessId;
	PUNICODE_STRING ProcessName;
	PKMODULE ModuleObject;

	ULONG32 ActiveThreads;
	LIST_ENTRY ActiveProcessLinks;

	HANDLE_TABLE ProcessHandleTable;

	VAD VadTree;
	ADDRESS_SPACE_DESCRIPTOR AddressSpace;

} KPROCESS, *PKPROCESS;

typedef VOID WAIT_PROCEDURE(
	__in PKTHREAD
);

typedef WAIT_PROCEDURE* PWAIT_PROCEDURE;

typedef struct _WAIT_OBJECT_HEADER {
	PWAIT_PROCEDURE SwitchProcedure;

} WAIT_OBJECT_HEADER, *PWAIT_OBJECT_HEADER;

typedef struct _WAIT_OBJECT_DELAY {
	WAIT_OBJECT_HEADER Header;

	ULONG64 StartTime;
	ULONG64 SleepTime;
} WAIT_OBJECT_DELAY, *PWAIT_OBJECT_DELAY;

typedef struct _WAIT_OBJECT_SPINLOCK {
	WAIT_OBJECT_HEADER Header;

	PKSPIN_LOCK LockQuerying;
} WAIT_OBJECT_SPINLOCK, *PWAIT_OBJECT_SPINLOCK;

typedef struct _KTCB {
	KTRAP_FRAME Registers;
	PADDRESS_SPACE_DESCRIPTOR AddressSpace;

	ULONG32 LogicalProcessor;
	UCHAR PrivilegeLevel;

	ULONG32 ThreadState;
	ULONG32 ThreadExitCode;

	LIST_ENTRY ScheduledThreads;

	PWAIT_OBJECT_HEADER WaitObject; //depends on ThreadState.
} KTCB, *PKTCB;

typedef struct _KTHREAD {

	struct {
		ULONG32 CallIndex;
		ULONG32 PreviousFlags;
		ULONG64 PreviousIp;
		ULONG64 PreviousStack;
	} SYSCALL;

	ULONG64 KernelStackBase;
	ULONG32 KernelStackSize;

	KTCB ThreadControlBlock;
	ULONG32 ActiveThreadId;
	LIST_ENTRY ActiveThreadLinks;

	ULONG64 UserStackBase;
	ULONG32 UserStackSize;

	//add locks.

	PKPROCESS Process;

} KTHREAD, *PKTHREAD;