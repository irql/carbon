


bits 64

section .text

%macro DECLARE_SYSTEM_SERVICE 2
global %1
export %1
%1:
	mov r10, rcx
	mov eax, %2
	syscall
	ret
%endmacro

DECLARE_SYSTEM_SERVICE NtGdiDisplayString, 0x10000000
DECLARE_SYSTEM_SERVICE NtGdiCreateConsole, 0x10000001
DECLARE_SYSTEM_SERVICE NtGdiReadConsole, 0x10000002
DECLARE_SYSTEM_SERVICE NtGdiWriteConsole, 0x10000003


