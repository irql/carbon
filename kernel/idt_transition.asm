;++
;
;Module Name:
;
;	idt_transition.asm
;
;Abstract:
;
;	Defines the interrupt descriptor table and basic procedures for
;	interrupts to transition.
;
;--

bits 64

section .text

global HalInterruptHandlerTable

extern HalHandleInterrupt

extern HalXsave
extern HalSimdSaveRegion

HalInitTrapFrame:
	push rax
	push rcx
	push rdx
	push rbx
	push rbp
	push rsi
	push rdi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15

	mov rax, ds
	push rax
	mov rax, 16
	mov ds, rax
	mov fs, rax
	mov gs, rax
	mov es, rax

	mov rdi, rsp

	sub rsp, 4096
	mov rcx, rsp

	and rsp, ~0x1f
	add rsp, 0x20
	
	mov rdx, HalXsave
	cmp qword [rdx], 0
	je .use_fxsave
	mov rdx, -1
	mov rax, -1
	xsave64 [rsp]
	jmp .fp_saved
	.use_fxsave:
	fxsave [rsp]
	.fp_saved:

	sub rsp, 40
	call HalHandleInterrupt
	add rsp, 40

	mov rdx, HalXsave
	cmp qword [rdx], 0
	je .use_fxrstor
	mov rdx, -1
	mov rax, -1
	xrstor64 [rsp]
	jmp .fp_restored
	.use_fxrstor:
	fxrstor [rsp]
	.fp_restored:
	mov rsp, rdi

	pop rax
	mov ds, rax
	mov fs, rax
	mov gs, rax
	mov es, rax

	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rdi
	pop rsi
	pop rbp
	pop rbx
	pop rdx
	pop rcx
	pop rax
	
	add rsp, 16
	iretq


%assign int_no 0
%rep 256
HalInt%+int_no:
	cli
	%if int_no != 8 && int_no != 10 && int_no != 11 && int_no != 12 && int_no != 13 && int_no != 14 && int_no != 17
	push 0
	%endif

	push 0%+int_no
	jmp HalInitTrapFrame
%assign int_no int_no+1
%endrep

HalInterruptHandlerTable:
%assign int_no 0
%rep 256
dq HalInt%+int_no
%assign int_no int_no+1
%endrep