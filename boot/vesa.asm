


BITS    16

%DEFINE     VIB_VIDEO_MODES 14
%DEFINE     VMI_WIDTH       18
%DEFINE     VMI_HEIGHT      20
%DEFINE     VMI_BPP         25
%DEFINE     VMI_LFB         40


VesaSetMode:

    ;
    ;   EAX = VESA info structure pointer.
    ;
    ;   set's a VESA mode
    ;

    pushad 
    mov     ecx, es
    push    ecx
    mov     ecx, fs
    push    ecx

    xor     cx, cx
    mov     es, cx
    mov     di, ax

    push    di
    mov     ax, 4f00h
    mov     di, 3000h
    int     10h
    pop     di

    cmp     ax, 4fh
    jne     .VesaFailure

    mov     dword es:[di+0], 1280
    mov     dword es:[di+4], 720
    mov     dword es:[di+8], 32

.VesaInitSearch:
    mov     ax, word [3000h+VIB_VIDEO_MODES]
    mov     si, ax
    mov     ax, word [3002h+VIB_VIDEO_MODES]
    mov     fs, ax

.VesaSearch:
    mov     cx, word fs:[si]
    add     si, 2

    cmp     cx, 0ffffh
    je      .VesaFailure

    push    di
    mov     ax, 4f01h
    mov     di, 4000h
    int     10h
    pop     di

    cmp     ax, 4fh
    jne     .VesaFailure

    mov     eax, dword es:[di+0]
    cmp     ax, word [4000h+VMI_WIDTH]
    jne     .VesaSearch

    mov     eax, dword es:[di+4]
    cmp     ax, word [4000h+VMI_HEIGHT]
    jne     .VesaSearch

    mov     eax, dword es:[di+8]
    cmp     al, byte [4000h+VMI_BPP]
    jne     .VesaSearch

    mov     eax, dword [4000h+VMI_LFB]
    mov     dword es:[di+12], eax

    mov     ax, 4f02h
    mov     bx, cx
    or      bx, 4000h
    xor     di, di
    int     10h

    cmp     ax, 4fh
    jne     .VesaFailure
    jmp     .VesaSuccess

.VesaFailure:

    cmp     dword es:[di+0], 1280
    jne     .VesaFailure1

    mov     dword es:[di+0], 640
    mov     dword es:[di+4], 480

    jmp     .VesaInitSearch

.VesaFailure1:
    mov     eax, 0D3ADB00Bh
    cli
    hlt

.VesaSuccess:

    pop     ecx
    mov     fs, ecx
    pop     ecx
    mov     es, ecx
    popad
    ret
    
