


#include "com.h"

VOID
KdCmdCrash(
	__in PKD_CMDR_CRASH Crash
)
{
	Crash;

	KdPrint( L"lol your shitty os crashed. @%d, exeception: %x\n", Crash->CpuIndex, Crash->Interrupt );

	KdPrint(
		L"RAX: %#.16I64x RBX: %#.16I64x RCX: %#.16I64x RDX: %#.16I64x\n"
		L"RSI: %#.16I64x RDI: %#.16I64x RBP: %#.16I64x RSP: %#.16I64x\n"
		L"R8 : %#.16I64x R9 : %#.16I64x R10: %#.16I64x R11: %#.16I64x\n"
		L"R12: %#.16I64x R13: %#.16I64x R14: %#.16I64x R15: %#.16I64x\n"
		L"RIP: %#.16I64x RFL: %#.8x\n"
		L"CR0: %#.8x CR2: %#.16I64x CR3: %#.16I64x CR4: %#.8x\n",
		Crash->Rax, Crash->Rbx, Crash->Rcx, Crash->Rdx,
		Crash->Rsi, Crash->Rdi, Crash->Rbp, Crash->Rsp,
		Crash->R8, Crash->R9, Crash->R10, Crash->R11,
		Crash->R12, Crash->R13, Crash->R14, Crash->R15,
		Crash->Rip, Crash->Rflags,
		Crash->Cr0, Crash->Cr2, Crash->Cr3, Crash->Cr4 );

}