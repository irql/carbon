

bits 16

ReadDap:
	.size db 0x10
	.zero db 0
	.read dw 0
    .ptr  dd 0
	.lba  dq 0

;edx:eax = logical block address.
;ebx = seg(upper), ofs(lower)
;cx = amount of sectors to read.
ReadSectors:
    pushad

    mov dword [ReadDap.lba], eax
    mov dword [ReadDap.lba+4], edx
    mov word [ReadDap.read], cx
    mov byte [ReadDap.size], 0x10
    mov dword [ReadDap.ptr], ebx

    mov ah, 0x42
    mov dl, byte [BootDisk]
    mov si, ReadDap
    int 0x13

    popad
    ret