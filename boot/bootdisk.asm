


BITS    16

BootDiskDap:
    db 10h      ;size
    db 0        ;zero
    dw 0        ;read
    dd 0        ;addr
    dq 0        ;lba

BootDiskReadSectors:
    pushad

    mov     dword [BootDiskDap+08h], eax
    mov     dword [BootDiskDap+0ch], edx
    mov     word  [BootDiskDap+02h], cx
    mov     byte  [BootDiskDap+00h], 10h
    mov     dword [BootDiskDap+04h], ebx

    mov     ah, 0x42
    mov     dl, byte [BootDisk]
    mov     si, BootDiskDap
    int     13h

    popad
    ret