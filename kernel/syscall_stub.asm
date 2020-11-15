

.CODE

PUBLIC KiFastSystemCall

EXTERNDEF KeServiceDescriptorTable:qword

EXTERNDEF KeProbeForRead:proc

KiFastSystemCall PROC FRAME

    .ENDPROLOG

	shl     r11, 32                     ; shift eflags
	or      rax, r11                    ; save eflags

	swapgs
	mov     r11, qword ptr gs:[4]       ; get the thread pointer.
	swapgs

	mov     qword ptr [r11], rax        ; save eflags & service code
	mov     qword ptr [r11+8], rcx      ; save user return
	mov     qword ptr [r11+16], rsp     ; save user stack

	mov     rsp, qword ptr [r11+24]     ; new kernel stack
	mov     ecx, dword ptr [r11+32]     ; add the top of it
	mov     ecx, ecx
	add     rsp, rcx

	sub     rsp, 8
	mov     dword ptr [rsp], 200202h    ; new eflags
	popfq

	mov     rcx, offset KeServiceDescriptorTable
	mov     qword ptr [rsp-8], rcx

	mov     rcx, rax                    ; get the service descriptor code 
	shr     rcx, 28
	and     rcx, 0Fh
	shl     rcx, 4
	add     rcx, qword ptr [rsp-8]      ; KeServiceDescriptorTable[ rcx >> 28 & 0xF ] 

	and     rax, 0FFFFFFFh
	cmp     eax, dword ptr [rcx]        ; compare against the ServiceCount
	jg      done

	mov     rcx, qword ptr [rcx+8]      ; get ServiceTable
    shl     rax, 1
	lea     rax, [rcx+rax*8]            ; Descriptor->ServiceTable[ rax ]
	
	movsxd  rcx, dword ptr [rax+8]
	test    ecx, ecx
	jz      call_service

	mov     r11, qword ptr [r11+16]     ; get user stack
	add     r11, 28h                    ; add home address reserve

	shl     rcx, 3
	add     r11, rcx
	sub     r11, 8

    push    rcx
	push    rdx
	push    r8
	push    r9
	push    r10
	push    r11
	push    rax


	mov     rdx, r11
	xchg    rcx, rdx

	call    KeProbeForRead
	
	pop     rax
	pop     r11
	pop     r10
	pop     r9
	pop     r8
	pop     rdx
	pop     rcx
    
    shr     rcx, 3                      ; contains ArgumentCount.

	copy_stack:
    push    qword ptr [r11]
    sub     r11, 8
    dec     rcx
    jnz     copy_stack

	call_service:

    sub     rsp, 20h
	mov     rcx, r10
	call    qword ptr [rax]

	done:
	swapgs
	mov     r11, qword ptr gs:[4]
	swapgs

	mov     rcx, qword ptr [r11+8]
	mov     rsp, qword ptr [r11+16]
	movsxd  r11, dword ptr [r11+4]
	sysretq

KiFastSystemCall ENDP


    ;swapgs
	;mov     r11, qword ptr gs:[4]       ; get the thread pointer.
	;swapgs

    ;push    1bh                         ; dummy ss
	;push    qword ptr [r11+16]          ; user stack
	;push    qword ptr [r11+4]           ; user eflags
    ;push    23h                         ; dummy cs
    ;push    qword ptr [r11+8]           ; user return
	;.PUSHFRAME                          ; machine frame for stack unwinding


END