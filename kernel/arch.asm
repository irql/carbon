;++
;
;Module Name:
;
;	arch.asm
;
;Abstract:
;
;	Defines architecture specific procedures.
;
;--

bits 64

section .text

global __interrupt
global __ltr
global __str
global __fninit

__interrupt:
	mov rdx, __interrupt.1
	mov byte [rdx+1], cl
.1:	int 0
	ret

__ltr:
	ltr cx
	ret

__str:
	str ax
	ret

__fninit:
	fninit
	ret
