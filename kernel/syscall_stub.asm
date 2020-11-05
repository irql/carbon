


bits 64

section .text

global KiFastSystemCall

extern KeServiceTableCount
extern KeServiceDescriptorTable

extern KeProbeForRead

KiFastSystemCall:

	shl r11, 32
	or rax, r11

	swapgs
	mov r11, qword [gs:4]
	swapgs

	mov qword [r11], rax
	mov qword [r11+8], rcx
	mov qword [r11+16], rsp

	mov rsp, qword [r11+24]
	mov ecx, dword [r11+32]
	mov ecx, ecx
	add rsp, rcx

	push qword 0x200202
	popfq

	;thank you nasm.
	mov rcx, KeServiceDescriptorTable
	mov qword [rsp-8], rcx
	mov rcx, KeServiceTableCount
	mov rcx, qword [rcx]
	mov qword [rsp-16], rcx

	mov rcx, rax
	shr rcx, 28
	and rcx, 0xF
	cmp rcx, qword [rsp-16]
	jge .done

	shl rcx, 4
	add rcx, qword [rsp-8]

	and rax, 0x0FFFFFFF
	cmp eax, dword [rcx]
	jg .done

	shl rax, 1
	mov rcx, qword [rcx+8]
	lea rax, [rcx+rax*8]
	
	movsx rcx, dword [rax+8]
	test ecx, ecx
	jz .call_service

	mov r11, qword [r11+16]
	add r11, 0x28

	shl rcx, 3
	add r11, rcx
	sub r11, 8

	push rcx
	push rdx
	push r8
	push r9
	push r10
	push r11
	push rax
	mov rdx, r11
	xchg rcx, rdx

	sub rsp, 0x28
	call KeProbeForRead
	add rsp, 0x28
	
	pop rax
	pop r11
	pop r10
	pop r9
	pop r8
	pop rdx
	pop rcx
	shr rcx, 3

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

.done:
	swapgs
	mov r11, qword [gs:4]
	swapgs

	mov rcx, qword [r11+8]
	mov rsp, qword [r11+16]
	movsx r11, dword [r11+4]
	o64 sysret
