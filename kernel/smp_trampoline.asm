;++
;
;Module Name:
;
;	smp_trampoline.asm
;
;Abstract:
;
;	AP trampoline procedures.
;
;--

;;
;;	this code wants a stack address (dword), page table (dword), cpu enabled flag (byte/bit)
;;

bits 16

section .text

global HalSmpTrampolineStart
extern HalSmpInitializeCpu

%define TRAMPOLINE_BASE 0x1000

align 0x1000

HalSmpTrampolineStart:
cli

xor ax, ax
mov ds, ax

mov es, ax
mov fs, ax
mov gs, ax
mov ss, ax

jmp 0:start - HalSmpTrampolineStart + TRAMPOLINE_BASE

start:

	mov eax, dword [0x1600]
	mov cr3, eax

	mov eax, cr4
	or eax, 1 << 5
	mov cr4, eax

	mov word [0x1200], 8*3-1
	mov dword [0x1202], 0x500

	lgdt [0x1200]

	mov ecx, 0xc0000080
	rdmsr
	or eax, 1 << 8
	wrmsr

	mov eax, cr0
	or eax, 1 << 31 | 1 << 0
	and eax, ~(0x60000000)
	mov cr0, eax

	mov ax, 16
	mov ds, ax

	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	jmp 8:long_mode - HalSmpTrampolineStart + TRAMPOLINE_BASE

bits 64

long_mode:

	mov rsp, qword [0x1800]
	mov rbp, rsp

	sub rsp, 0x28
	mov rcx, qword [0x1808]
	jmp rcx

	hlt