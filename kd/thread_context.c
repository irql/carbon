


#include "com.h"

VOID
KdCmdThreadContext(
	__in PKD_CMDR_THREAD_CONTEXT Cmd
)
{

	KdPrint(
		L"RAX: %#.16I64x RBX: %#.16I64x RCX: %#.16I64x RDX: %#.16I64x\n"
		L"RSI: %#.16I64x RDI: %#.16I64x RBP: %#.16I64x RSP: %#.16I64x\n"
		L"R8 : %#.16I64x R9 : %#.16I64x R10: %#.16I64x R11: %#.16I64x\n"
		L"R12: %#.16I64x R13: %#.16I64x R14: %#.16I64x R15: %#.16I64x\n"
		L"RIP: %#.16I64x RFL: %#.8x CS : %#.4x DS : %#.4x\n",
		Cmd->Rax, Cmd->Rbx, Cmd->Rcx, Cmd->Rdx,
		Cmd->Rsi, Cmd->Rdi, Cmd->Rbp, Cmd->Rsp,
		Cmd->R8, Cmd->R9, Cmd->R10, Cmd->R11,
		Cmd->R12, Cmd->R13, Cmd->R14, Cmd->R15,
		Cmd->Rip, Cmd->EFlags,
		Cmd->CodeSegment, Cmd->DataSegment );

}