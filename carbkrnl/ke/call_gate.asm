


.CODE

PUBLIC      KiFastSystemCall
EXTERNDEF   KiServiceDescriptorTable:QWORD
EXTERNDEF   KeProbeForRead:PROC

; KPCB
ThreadQueue =           168

; KTHREAD
ServiceNumber =         8
PreviousEFlags =        12
PreviousIp =            16
PreviousStack =         24
KernelStackBase =       32
KernelStackLength =     40
SyscallActive =         03d1h
PreviousMode =          03d0h

; KSSDT
ServiceCount =          0
ServiceTable =          8

; KSYSTEM_SERVICE
Procedure =             0
ArgumentCount =         8

ALIGN       16
KiFastSystemCall PROC FRAME

    ;TODO: change swapgs for readmsr, this instruction is fucking retarded

    .ENDPROLOG
    shl     r11, 32
    mov     eax, eax
    or      rax, r11
    cli
    swapgs
    mov     r11, qword ptr gs:[ThreadQueue]     ; Thread pointer.
    swapgs
    sti
    mov     qword ptr [r11+ServiceNumber], rax  ; Save EFlags and service code
    mov     qword ptr [r11+PreviousIp], rcx     ; Save user return address
    mov     qword ptr [r11+PreviousStack], rsp  ; Save user stack
    mov     rsp, qword ptr [r11+KernelStackBase] ; Thread kernel stack
    mov     ecx, dword ptr [r11+KernelStackLength] ; Get the top of the stack
    add     rsp, rcx
    mov     byte ptr [r11+SyscallActive], 1
    mov     byte ptr [r11+PreviousMode], 1      ; Thread->PreviousMode = UserMode
                                                ; isn't actually necessary, previousmode is 
                                                ; set when the thread is created.
    sub     rsp, 8
    mov     dword ptr [rsp], 202h
    popfq

    mov     rcx, offset KiServiceDescriptorTable
    mov     qword ptr [rsp-8], rcx

    mov     ecx, eax                            ; Get the service descriptor code
    shr     rcx, 28
    and     rcx, 0fh
    shl     rcx, 4
    add     rcx, qword ptr [rsp-8]              ; KiServiceDescriptorTable[ rcx >> 28 & 0x0F ]

    and     eax, 0FFFFFFFh                      ; Get the service number
    cmp     eax, dword ptr [rcx+ServiceCount]
    jge     KiProcedureFinished                 ; if ( ServiceNumber >= Kssdt.ServiceCount )

    mov     rcx, qword ptr [rcx+ServiceTable]   ; Get the service table
    shl     rax, 1                              ; ServiceNumber *= 2
    lea     rax, [rcx+rax*8]                    ; Kssdt.ServiceTable[ ServiceNumber ]

    mov     ecx, dword ptr [rax+ArgumentCount]
    test    ecx, ecx
    jz      KiCallService

    mov     r11, qword ptr [r11+PreviousStack]
    add     r11, 28h
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
    sub     rsp, 28h
    call    KeProbeForRead
    add     rsp, 28h

    pop     rax
    pop     r11
    pop     r10
    pop     r9
    pop     r8
    pop     rdx
    pop     rcx
    shr     rcx, 3

KiCopyStack:
    push    qword ptr [r11]
    sub     r11, 8
    dec     rcx
    jnz     KiCopyStack

KiCallService:
    sub     rsp, 20h
    mov     rcx, r10
    call    qword ptr [rax]
    
KiProcedureFinished:
    cli
    swapgs 
    mov     r11, qword ptr gs:[ThreadQueue]
    swapgs
    sti
    mov     byte ptr [r11+SyscallActive], 0
    mov     rcx, qword ptr [r11+PreviousIp]
    mov     rsp, qword ptr [r11+PreviousStack]
    mov     r11d, dword ptr [r11+PreviousEFlags]
    sysretq

KiFastSystemCall ENDP

END