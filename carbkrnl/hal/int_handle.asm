


BITS        64

SECTION     .text

GLOBAL      KxIntHandlerTable
GLOBAL      KiInterruptHandleBase

GLOBAL      KxHardwareInterrupt
GLOBAL      KxExceptionInterrupt
GLOBAL      KxSpuriousInterrupt
GLOBAL      KxIpiInterrupt

EXTERN      HalEoi

EXTERN      KiHardwareDispatch
EXTERN      KiExceptionDispatch
EXTERN      KiSwapContext
EXTERN      KiIpiCall

;
; Huuuuuuuuuuge bug. the stack is not aligned.
;

%DEFINE     KxTrapFrameLength 800

%MACRO      KiPushTrapFrame 0
    push    rax
    push    rcx
    push    rdx
	push    rbx
	push    rbp
	push    rsi
	push    rdi
	push    r8
	push    r9
	push    r10
	push    r11
	push    r12
	push    r13
	push    r14
	push    r15

    mov     rax, ds
    push    rax
    mov     rax, es
    push    rax
    mov     rax, fs
    push    rax
    mov     rax, gs
    push    rax

    mov     rax, 16
    mov     ds, rax
    mov     es, rax
    mov     fs, rax
    mov     gs, rax
    mov     es, rax

    mov     rax, cr3
    push    rax

    mov     rax, dr0
    push    rax
    mov     rax, dr1
    push    rax
    mov     rax, dr2
    push    rax
    mov     rax, dr3
    push    rax
    mov     rax, dr6
    push    rax
    mov     rax, dr7
    push    rax

    sub     rsp, 512 + 24
    fxsave64 [rsp]
%ENDMACRO 

%MACRO      KiPopTrapFrame 0
    fxrstor64 [rsp]
    add     rsp, 512 + 24

    pop     rax
    mov     dr7, rax
    pop     rax
    mov     dr6, rax
    pop     rax
    mov     dr3, rax
    pop     rax
    mov     dr2, rax
    pop     rax
    mov     dr1, rax
    pop     rax
    mov     dr0, rax

    pop     rax
    mov     cr3, rax

    pop     rax
    mov     gs, rax
    pop     rax
    mov     fs, rax
    pop     rax
    mov     es, rax
    pop     rax
    mov     ds, rax

    pop     r15
	pop     r14
	pop     r13
	pop     r12
	pop     r11
	pop     r10
	pop     r9
	pop     r8
	pop     rdi
	pop     rsi
	pop     rbp
	pop     rbx
	pop     rdx
	pop     rcx
	pop     rax
%ENDMACRO

ALIGN       16
KxHardwareInterrupt:
    KiPushTrapFrame
    mov     rcx, rsp
    sub     rsp, 28h
    call    KiHardwareDispatch
    call    HalEoi
    add     rsp, 28h
    KiPopTrapFrame
	add     rsp, 16
	iretq

ALIGN       16
KxExceptionInterrupt:
    sti
    KiPushTrapFrame
    mov     rcx, rsp
    sub     rsp, 28h
    call    KiExceptionDispatch
    call    HalEoi
    add     rsp, 28h
    KiPopTrapFrame
    add     rsp, 16
    iretq

ALIGN       16
KxSpuriousInterrupt:
    sub     rsp, 28h
    call    HalEoi
    add     rsp, 28h + 16
    iretq

ALIGN       16
KxIpiInterrupt:
    KiPushTrapFrame
    mov     rcx, rsp
    sub     rsp, 28h
    call    KiIpiCall
    call    HalEoi
    add     rsp, 28h
    KiPopTrapFrame
    add     rsp, 16
    iretq

ALIGN       16
KxSwapContext:
    KiPushTrapFrame
    mov     rcx, rsp
    sub     rsp, 28h
    call    KiSwapContext
    call    HalEoi
    add     rsp, 28h
    KiPopTrapFrame
    add     rsp, 16
    iretq

ALIGN       16
%ASSIGN     VECTOR 0
%REP        256
KxHandleInt%+VECTOR:
    cli
    %IF     VECTOR != 8 && \
            VECTOR != 10 && \
            VECTOR != 11 && \
            VECTOR != 12 && \
            VECTOR != 13 && \
            VECTOR != 14 && \
            VECTOR != 17
    push    0
    %ENDIF
    push    0%+VECTOR
    jmp     qword [KiInterruptHandleBase + (0%+VECTOR * 8)]
%ASSIGN     VECTOR VECTOR+1
%ENDREP

SECTION     .data

;
; The actual table of handlers for each vector, excluding the stubs
; which are inside HalIntHandlerTable
;

ALIGN       16
KiInterruptHandleBase:
%REP        32
dq          KxExceptionInterrupt
%ENDREP

dq          KxSwapContext

dq          KxHardwareInterrupt
dq          KxHardwareInterrupt
dq          KxHardwareInterrupt
dq          KxHardwareInterrupt
dq          KxHardwareInterrupt
dq          KxHardwareInterrupt
dq          KxHardwareInterrupt
dq          KxHardwareInterrupt
dq          KxExceptionInterrupt ; 0x29 - fast fail
dq          KxHardwareInterrupt
dq          KxHardwareInterrupt
dq          KxExceptionInterrupt ; 0x2c - assert

%REP        209
dq          KxHardwareInterrupt
%ENDREP

dq          KxIpiInterrupt
dq          KxSpuriousInterrupt

SECTION     .rdata

;
; Table of raw handlers for the idt
;

KxIntHandlerTable:
%ASSIGN     VECTOR 0
%REP        256
dq          KxHandleInt%+VECTOR
%ASSIGN     VECTOR VECTOR+1
%ENDREP

