


.CODE

PUBLIC  setjmp
PUBLIC  longjmp

ALIGN   16
setjmp PROC EXPORT
    ;int     29h
    ret
setjmp ENDP

ALIGN   16
longjmp PROC EXPORT
    int     29h
    ret
longjmp ENDP

END