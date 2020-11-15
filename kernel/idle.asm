

.CODE

PUBLIC KiIdleThread

KiIdleThread PROC

i:  hlt
	jmp     i

KiIdleThread ENDP


end