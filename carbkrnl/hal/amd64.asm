


BITS 64

SECTION     .text

GLOBAL      __ltr
GLOBAL      __str
GLOBAL      __interrupt
;GLOBAL      KiLeaveQuantumEarly


__interrupt:
	mov     rdx, __interrupt.0
	mov     byte [rdx+1], cl
.0:	int     0
	ret

__ltr:
    ltr     cx
    ret

__str:
    str     ax
    ret


KiLeaveQuantumEarly:
    mov     rax, cr8
    cmp     rax, 2
    jl      @p
    mov     rax, 0
    mov     cr8, rax
    int     0x3
@p:
    int     0x20
    ret
