

.CODE

PUBLIC KiFastSystemCall

EXTERNDEF KeServiceDescriptorTable:qword

EXTERNDEF KeProbeForRead:proc

KiFastSystemCall PROC FRAME

	.ENDPROLOG

	shl r11, 32
	or rax, r11

	swapgs
	mov r11, qword ptr gs:[4]
	swapgs

	mov qword ptr [r11], rax
	mov qword ptr [r11+8], rcx
	mov qword ptr [r11+16], rsp

	mov rsp, qword ptr [r11+24]
	mov ecx, dword ptr [r11+32]
	mov ecx, ecx
	add rsp, rcx

	sub rsp, 8
	mov dword ptr [rsp], 200202h
	popfq

	mov rcx, offset KeServiceDescriptorTable
	mov qword ptr [rsp-8], rcx

	mov rcx, rax
	shr rcx, 28
	and rcx, 0fh
	shl rcx, 4
	add rcx, qword ptr [rsp-8]

	and rax, 0FFFFFFFh
	cmp eax, dword ptr [rcx]
	jg done

	shl rax, 1
	mov rcx, qword ptr [rcx+8]
	lea rax, [rcx+rax*8]
	
	movsxd rcx, dword ptr [rax+8]
	test ecx, ecx
	jz call_service

	mov r11, qword ptr [r11+16]
	add r11, 28h

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

	sub rsp, 28h
	call KeProbeForRead
	add rsp, 28h
	
	pop rax
	pop r11
	pop r10
	pop r9
	pop r8
	pop rdx
	pop rcx
	shr rcx, 3

	copy_stack:
	push qword ptr [r11]

	sub r11, 8
	dec rcx
	jnz copy_stack

	call_service:
	sub rsp, 20h

	mov rcx, r10
	call qword ptr [rax]
	
	;imagine fixing the stack lol.

	done:
	swapgs
	mov r11, qword ptr gs:[4]
	swapgs

	mov rcx, qword ptr [r11+8]
	mov rsp, qword ptr [r11+16]
	movsxd r11, dword ptr [r11+4]
	sysretq

KiFastSystemCall ENDP


END