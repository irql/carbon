


.CODE

PUBLIC      PspSystemThreadReturn
EXTERNDEF   ZwTerminateThread:PROC

ALIGN       16
PspSystemThreadReturn PROC FRAME
    .ENDPROLOG
    mov     rcx, -2
    mov     rdx, rax
    sub     rsp, 28h
    call    ZwTerminateThread
    int     3
PspSystemThreadReturn ENDP

END