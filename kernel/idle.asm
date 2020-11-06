

.CODE

PUBLIC KiIdleThread

KiIdleThread PROC

idle:hlt
	jmp idle

KiIdleThread ENDP


end