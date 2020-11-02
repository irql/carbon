


bits 64

section .text

global KiFastSystemCall
extern KeServiceDescriptorTable


KiFastSystemCall:
	
	cmp rax, 2 ;this value is the max syscall.
	jle KiFastSystemCall.valid
	
	mov rax, -1
	ret

	.valid:

	shl r11, 32
	or rax, r11

	swapgs
	mov r11, qword [gs:4]
	swapgs

	mov qword [r11], rax
	mov qword [r11+8], rcx
	mov qword [r11+16], rsp
	
	;new stack.
	mov rsp, qword [r11+24]
	movsx rcx, dword [r11+32]
	add rsp, rcx

	push qword 0x200202
	popfq

	mov rcx, KeServiceDescriptorTable
	mov eax, eax
	mov rax, qword [rcx+rax*8]
	mov rcx, r10
	sub rsp, 0x40
	call rax
	add rsp, 0x40

	swapgs
	mov r11, qword [gs:4]
	swapgs

	mov rcx, qword [r11+8]
	mov rsp, qword [r11+16]
	movsx r11, dword [r11+4]
	
	db 0x48
	db 0x0f
	db 0x07
	;sysretq
	;o64 sysret