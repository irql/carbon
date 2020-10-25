
;expects 16 bit execution mode.
;expects int 13h boot id to be in var called BootDisk
;expects bpb definitions (or at least their addresses 0x7c03 - 0x7cxx)

bits 16

%define CLUSTER_LOW		26
%define CLUSTER_HIGH	20

GetFirstDataSector:
    ;(Bpb->Dos2_00Bpb.ReservedSectors + 
    ;   Bpb->Dos3_31Bpb.HiddenSectors + (
    ;       Bpb->Dos2_00Bpb.FatCount * 
    ;       Bpb->Dos7_01Bpb.SectorsPerFat))
    push ebx
    push edx

    xor ax, ax
    mov al, byte [FatCount]
    mul word [SectorsPerFat32]

    mov bx, dx
    shl ebx, 16
    mov bx, ax

    add bx, word [ReservedSectors]
    add ebx, dword [HiddenSectors]

    mov eax, ebx
    pop edx
    pop ebx
    ret


;edx = clusnum
;ebx = seg(upper), ofs(lower)
ReadCluster:
    pushad
    push ebx

    call GetFirstDataSector
    mov ebx, eax

    dec edx
    dec edx

    xor eax, eax
    mov al, byte [SectorsPerCluster]
    mul edx
    add eax, ebx
    ;edx:eax contains sector number of cluster.

    pop ebx
    xor cx, cx
    mov cl, byte [SectorsPerCluster]
    call ReadSectors

    popad
    ret



;routine uses 0x4000 for a temporary buffer.
;prints error & halts on failure.

;eax = directory in memory
;ebx = seg(upper), ofs(lower)
;cx = file name ptr
ReadDirectoryFile:
    pushad

    push ebx
    mov bx, cx

    mov di, ax
    shr eax, 16
    mov es, ax

    mov ax, 0
    mov ds, ax

    ;es:di is now the directory ptr.
    .NextFileEntry:
    add di, 32
    mov si, bx

    cmp byte [es:di], 0
    je .FileEntryNotFound

    push edi
    mov cx, 11
    repe cmpsb
    pop edi

    jne .NextFileEntry

    mov dx, word [es:di + CLUSTER_HIGH]
    shl edx, 16
    mov dx, word [es:di + CLUSTER_LOW]
    ;edx = cluster

    pop ebx
    .FollowChain:

    ;edx
    ;ebx
    call ReadCluster

    push ebx

    mov eax, 4
    mul edx
    movzx ecx, word [BytesPerSector]
    div ecx
    ;eax = FatSectorOffset
    ;edx = FatSectorIndex

    push edx
    xor edx, edx

    add ax, word [ReservedSectors]
    add eax, dword [HiddenSectors]
    mov ebx, 0x4000
    mov cx, 1
    call ReadSectors

    pop edx
    mov edx, dword [0x4000+edx]
    push edx

    movzx ax, byte [SectorsPerCluster]
    mov cx, word [BytesPerSector]
    mul cx
    ;dx:ax = SectorsPerCluster * BytesPerSector
    mov bx, 0x10
    div bx
    ;dx:ax = (SectorsPerCluster * BytesPerSector) / 0x10

    mov cx, dx
    shr ecx, 16
    mov cx, ax
    shl ecx, 16
    
    pop edx
    pop ebx
    add ebx, ecx

    cmp edx, 0x0fffffff
    jne .FollowChain
    jmp .FileReadDone
    .FileEntryNotFound:
    mov ax, 0xb800
    mov es, ax
    xor di, di

    mov cx, 11
    .print1:

    mov dl, byte [si]
    mov dh, 0x0f

    mov word [es:di], dx
    
    inc di
    inc di
    inc si
    dec cx
    jnz .print1

    mov si, ErrNotFound
    call ErrHandle
    
    cli
    hlt
    .FileReadDone:

    xor ax, ax
    mov es, ax
    popad
    ret

