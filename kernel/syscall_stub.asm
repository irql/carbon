


bits 64

section .text

global KiFastSystemCall
extern KeServiceDescriptorTable


KiFastSystemCall:
	
	cmp rax, 8 ;this value is the max syscall.
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
	mov ecx, dword [r11+32]
	mov ecx, ecx
	add rsp, rcx

	push qword 0x200202
	popfq

	mov rcx, KeServiceDescriptorTable
	mov eax, eax
	shl rax, 1
	lea rax, [rcx+rax*8]
	
	movsx rcx, dword [rax+8]
	test ecx, ecx
	jz .call_service

	mov r11, qword [r11+16]
	add r11, 0x28

	shl rcx, 3
	add r11, rcx
	shr rcx, 3
	sub r11, 8

	.copy_stack:
	push qword [r11]

	sub r11, 8
	dec rcx
	jnz .copy_stack

	.call_service:
	sub rsp, 0x20
	mov rcx, r10
	call qword [rax]
	
	;imagine fixing the stack lol.

	swapgs
	mov r11, qword [gs:4]
	swapgs

	mov rcx, qword [r11+8]
	mov rsp, qword [r11+16]
	movsx r11, dword [r11+4]
	o64 sysret
