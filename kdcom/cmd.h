

#pragma once

#pragma warning( disable : 4200 )

#define KD_ACK_BYTE			(0xFA)

#define KD_CMD_CONNECT      (0x00)
#define KD_CMD_BREAK        (0x01)
#define KD_CMD_CONTINUE     (0x02)
#define KD_CMD_CRASH        (0x03)
#define KD_CMD_MESSAGE      (0x04)
#define KD_CMD_LIST_THREADS (0x05)
#define KD_CMD_LIST_MODULES (0x06)

typedef struct _KB_BASE_COMMAND_RECIEVE {
	UCHAR KdAckByte;
	UCHAR KdCommandByte;

	ULONG32 KdCmdSize;

} KD_BASE_COMMAND_RECIEVE, *PKD_BASE_COMMAND_RECIEVE;

typedef struct _KD_BASE_COMMAND_SEND {
	UCHAR KdAckByte;
	UCHAR KdCommandByte;

} KD_BASE_COMMAND_SEND, *PKD_BASE_COMMAND_SEND;

typedef struct _KD_CMDR_CRASH {
	KD_BASE_COMMAND_RECIEVE Base;

	ULONG32 CpuIndex;

	//TRAPFRAME + 4096.
	ULONG64 R15, R14, R13, R12, R11, R10, R9, R8, Rdi, Rsi, Rbp, Rbx, Rdx, Rcx, Rax;
	ULONG64 Interrupt, Error;
	ULONG64 Rip, CodeSegment, Rflags, Rsp, StackSegment;

	ULONG64 Cr0, Cr2, Cr3, Cr4;
} KD_CMDR_CRASH, *PKD_CMDR_CRASH;

typedef struct _KD_CMDR_MESSAGE {
	KD_BASE_COMMAND_RECIEVE Base;

	WCHAR Message[ 0 ];
} KD_CMDR_MESSAGE, *PKD_CMDR_MESSAGE;

typedef struct _KD_THREAD {
	ULONG32 ProcessId;
	ULONG32 ThreadId;

} KD_THREAD, *PKD_THREAD;

typedef struct _KD_CMDR_LIST_THREADS {
	KD_BASE_COMMAND_RECIEVE Base;

	KD_THREAD Thread[ 0 ];

} KD_CMDR_LIST_THREADS, *PKD_CMDR_LIST_THREADS;

typedef struct _KD_MODULE {
	ULONG64 ModuleStart;
	ULONG64 ModuleEnd;
	WCHAR ModuleName[ 32 ];

} KD_MODULE, *PKD_MODULE;

typedef struct _KD_CMDR_LIST_MODULES {
	KD_BASE_COMMAND_RECIEVE Base;

	KD_MODULE Module[ 0 ];
} KD_CMDR_LIST_MODULES, *PKD_CMDR_LIST_MODULES;
