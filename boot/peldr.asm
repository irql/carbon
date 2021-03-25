


BITS    64

%DEFINE sizeof_FileHeader			14h
%DEFINE sizeof_SectionHeader		28h
%DEFINE sizeof_DataDirectory		8h
%DEFINE sizeof_BaseReloc			8h

%DEFINE dos_e_lfanew				3ch

%DEFINE nt_FileHeader				4h
%DEFINE nt_OptionalHeader			nt_FileHeader+sizeof_FileHeader

%DEFINE fh_SizeOfOptionalHeader		10h
%DEFINE fh_NumberOfSections			2h

%DEFINE oh_AddressOfEntryPoint		10h
%DEFINE oh_ImageBase				18h
%DEFINE oh_SizeOfHeaders			3ch
%DEFINE oh_DataDirectory			70h

%DEFINE sh_VirtualSize				8h
%DEFINE sh_VirtualAddress			0ch
%DEFINE sh_RawSize					10h
%DEFINE sh_RawAddress				14h
%DEFINE sh_Characteristics			24h

%DEFINE br_VirtualAddress			0h
%DEFINE br_SizeOfBlock				4h

ImageLdrLoadFile:
    
    ;
    ;   RCX = Base address of file in memory.
    ;
    ;   returns:
    ;   RAX = entry point
    ;

    mov     qword [rsp+8], rcx
    mov     qword [rsp+16], rdi
    mov     qword [rsp+24], rsi
    sub     rsp, 56

    mov     rsi, rcx
    mov     r10d, dword [rsi+dos_e_lfanew]
    add     r10, rsi
    movzx   r11, word [r10+nt_FileHeader+fh_SizeOfOptionalHeader]
    lea     r11, [r11+r10+nt_OptionalHeader]
    mov     ecx, dword [r10+nt_OptionalHeader+oh_SizeOfHeaders]
    mov     rdi, 0ffffffffffe00000h

    ;
    ;   we now have 
    ;   RSI = DosHeader
    ;   R10 = NtHeader
    ;   R11 = SectionHeaders
    ;   RCX = SizeOfHeaders
    ;   RDI = ImageBase
    ;

    mov     qword [rsp], rdi
    mov     qword [rsp+8], rsi
    mov     qword [rsp+16], r10
    
    cld
    rep     movsb
    
    mov     rsi, qword [rsp+8]
    mov     rdi, qword [rsp]

    movzx   rdx, word [r10+nt_FileHeader+fh_NumberOfSections]
    xor     rcx, rcx
.LoadSection:
    mov     qword [rsp+40], rcx
    imul    rcx, sizeof_SectionHeader

    lea     rax, [r11+rcx]

    mov     rdi, qword [rsp]
    mov     rsi, qword [rsp+8]
    mov     ecx, dword [rax+sh_VirtualAddress]
    add     rdi, rcx
    mov     ecx, dword [rax+sh_RawAddress] 
    add     rsi, rcx

    mov     qword [rsp+48], rax
    cld
    mov     ecx, dword [rax+sh_VirtualSize] 
    mov     qword [rsp+24], rdi
    mov     qword [rsp+32], rsi
    xor     rax, rax
    rep     stosb
    mov     rax, qword [rsp+48]
    mov     rsi, qword [rsp+32]
    mov     rdi, qword [rsp+24]
    mov     ecx, dword [rax+sh_RawSize]
    rep     movsb

    mov     rcx, qword [rsp+40]
    inc     rcx
    cmp     rcx, rdx
    jne     .LoadSection
    
    mov     rdi, qword [rsp]
    mov     rsi, qword [rsp+8]
    mov     r10, qword [rsp+16]
    mov     rcx, rdi
    sub     rcx, qword [r10+nt_OptionalHeader+oh_ImageBase]
    mov     qword [rsp+40], rcx

    mov     edx, dword [r10+nt_OptionalHeader+oh_DataDirectory+(5*sizeof_DataDirectory)]
    test    edx, edx
    jz      .ImageLdrDone
    add     rdx, rdi

    ;
    ;   prepare for base relocation
    ;   RCX = BaseDelta
    ;   RDX = BaseReloc
    ;
    
.ImageReloc:
    cmp     dword [rdx+br_VirtualAddress], 0
    je      .ImageRelocSuccess
    
    mov     r11d, dword [rdx+br_SizeOfBlock]
    cmp     r11d, sizeof_BaseReloc
    jle     .ImageRelocNextEntry

    sub     r11, sizeof_BaseReloc
    lea     r8, [rdx+sizeof_BaseReloc]
.ImageRelocResolveThunk:
    cmp     word [r8], 0
    je      .ImageRelocFinishThunk

    movzx   r10, word [r8]
    and     r10d, 0fffh
    add     r10d, dword [rdx+br_VirtualAddress] 
    lea     r9, [rdi+r10]
    add     qword [r9], rcx

.ImageRelocFinishThunk:
    add     r8, 2
    sub     r11, 2
    jz      .ImageRelocNextEntry
    jmp     .ImageRelocResolveThunk

.ImageRelocNextEntry:
    mov     r8d, dword [rdx+br_SizeOfBlock]
    add     rdx, r8
    jmp     .ImageReloc

.ImageRelocSuccess:
.ImageLdrDone:
    mov     r10, qword [rsp+16]
    mov     eax, dword [r10+nt_OptionalHeader+oh_AddressOfEntryPoint]
    add     rax, rdi

    add     rsp, 56
    mov     rdi, qword [rsp+16]
    mov     rsi, qword [rsp+24]
    ret