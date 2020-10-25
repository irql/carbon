


bits 64

section .text

global KiIdleThread

KiIdleThread:
.Z:	hlt
	jmp KiIdleThread.Z
