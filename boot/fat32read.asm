

BITS    16

%DEFINE CLUSTER_LO      26
%DEFINE CLUSTER_HI      20

BootFsFat32InternalGetFirstClusterSector:
    
    ;
    ;   evaluates the formula below, taken
    ;   from fatgen103.
    ;
    ;   Bpb->Dos2_00Bpb.ReservedSectors + 
    ;   Bpb->Dos3_31Bpb.HiddenSectors + 
    ;   Bpb->Dos2_00Bpb.FatCount * 
    ;   Bpb->Dos7_01Bpb.SectorsPerFat
    ;

    push    ebx
    push    edx

    movzx   ax, byte [FatCount]
    mul     word [SectorsPerFat32]

    mov     bx, dx
    shl     ebx, 16
    mov     bx, ax

    add     bx, word [ReservedSectors]
    add     ebx, dword [HiddenSectors]

    mov     eax, ebx
    pop     edx
    pop     ebx
    ret

BootFsFat32ReadCluster:

    ;
    ;   EDX = ClusterNumber
    ;   EBX = Segment:Offset
    ;   Read's the cluster specified by EDX
    ;   into the buffer specified by EBX.
    ;

    pushad
    push    ebx

    call    BootFsFat32InternalGetFirstClusterSector
    mov     ebx, eax

    ;
    ;   the fat32, 2 cluster sub
    ;

    sub     edx, 2

    ;
    ;   calculate FirstCluster + SectorsPerCluster * ClusterNumber
    ;

    movzx   eax, byte [SectorsPerCluster]
    mul     edx
    add     eax, ebx

    pop     ebx
    movzx   cx, byte [SectorsPerCluster]
    call    BootDiskReadSectors
    
    popad
    ret
    
BootFsFat32ReadClusterChain:

    ;
    ;   EDX = ClusterNumber
    ;   EBX = Segment:Offset
    ;
    ;   returns:
    ;   EBX = Segment:Offset of next free memory
    ;         region.
    ;
    ;   Reads a chain of clusters.
    ;

    pushad

    .ClusterChain:
    
    call    BootFsFat32ReadCluster
    push    ebx

    ;
    ;   calculates the EDX = FatSectorIndex and
    ;   EAX = FatSectorOffset 
    ;

    mov     eax, 4
    mul     edx
    movzx   ecx, word [BytesPerSector]
    div     ecx

    ;
    ;   this code reads the FatSectorIndex into memory
    ;

    push    edx
    xor     edx, edx
    
    add     ax, word [ReservedSectors]
    add     eax, dword [HiddenSectors]
    mov     ebx, 4000h
    mov     cx, 1
    call    BootDiskReadSectors

    ;
    ;   load the new cluster number in edx
    ;   and use (SectorsPerCluster * BytesPerSector) / 0x10 to 
    ;   calculate a new segment:offset pointer. (we're currently
    ;   ignoring the offset field of segment:offset pointers)
    ;

    pop     edx
    mov     edx, dword [edx+4000h]
    push    edx

    movzx   eax, byte [SectorsPerCluster]
    mov     cx, word [BytesPerSector]
    mul     cx
    mov     bx, 16
    div     bx

    mov     ecx, eax
    shl     ecx, 16

    pop     edx
    pop     ebx
    add     ebx, ecx
    cmp     edx, 0x0fffffff
    jne     .ClusterChain

    mov     dword [esp+16], ebx
    popad
    ret

BootFsFat32ReadDirectoryFile:

    ;
    ;   EAX = Directory
    ;   EBX = Segment:Offset
    ;    CX = FileNameOffset
    ;   reads a fat32 file from a directory
    ;   uses 4000h as a temporary buffer.
    ;

    pushad
    mov     ax, es
    push    eax
    push    ebx
    mov     bx, cx

    mov     di, ax
    shr     eax, 16
    mov     es, ax

    ;
    ;   check each entry in the directory
    ;   comparing the 11 char name to the one
    ;   passed to this function, if the first byte
    ;   is 0, we can assume that is the end of the 
    ;   directory listing, and jump to a failure
    ;   handler.
    ;

.DirectoryEntry:
    add     di, 32
    mov     si, bx
    cmp     byte es:[di], 0
    je      .FileReadFailure

    push    edi
    mov     cx, 11
    repe    cmpsb
    pop     edi
    jne     .DirectoryEntry

    ;
    ;   the code escaped the directory loop,
    ;   meaning it must've found our entry.
    ;   EDX = ClusterNumber
    ;

    mov     dx, word es:[di + CLUSTER_HI]
    shl     edx, 16
    mov     dx, word es:[di + CLUSTER_LO]

    pop     ebx

    call    BootFsFat32ReadClusterChain
    jmp     .FileReadSuccess

.FileReadFailure:
    mov eax, 0D3ADB00Bh
    cli
    hlt
.FileReadSuccess:
    
    pop     eax
    mov     es, ax
    mov     dword [esp+16], ebx
    popad
    ret


BootFsFat32ReadRdDirectory:
    
    ;
    ;   EAX = InitrdDirectoryBase
    ;   EBX = InitrdFileReadPointer
    ;   ECX = InitrdFileList
    ;   
    ;   This code should load the entirety of the initrd 
    ;   directory passed as a parameter, into a list of 
    ;   RD_FILE_LIST_ENTRY's 
    ;

    pushad
    mov     dx, es
    push    edx

    mov     di, ax
    shr     eax, 16
    mov     es, ax
    add     di, 32

    mov     ebp, ecx
    mov     dword [ebp], 0
    add     ecx, 4

.DirectoryEntry:
    add     di, 32

    cmp     byte es:[di], 0
    je      .DirectorySuccess
    cmp     byte es:[di], 0xE5
    je      .DirectoryEntry

    inc     dword [ebp]

    mov     edx, dword es:[di]
    mov     dword [ecx], edx
    mov     edx, dword es:[di+4]
    mov     dword [ecx+4], edx
    mov     edx, dword es:[di+8]
    mov     dword [ecx+8], edx
    mov     byte [ecx+11], 0

    mov     dx, word es:[di + CLUSTER_HI]
    shl     edx, 16
    mov     dx, word es:[di + CLUSTER_LO]
    
    mov     dword [ecx+12], ebx
    call    BootFsFat32ReadClusterChain
    mov     dword [ecx+16], ebx
    mov     edx, dword [ecx+12]
    sub     dword [ecx+16], edx

    add     ecx, 20

    jmp     .DirectoryEntry
.DirectorySuccess:

    pop     eax
    mov     es, ax
    popad
    ret
