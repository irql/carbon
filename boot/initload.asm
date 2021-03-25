


BITS    16
ORG     7c00h

jmp     InitLoad
nop

dos_2_00_bpb:
FileSystemIdentifier        db "MSDOS5.0"
BytesPerSector              dw 512
SectorsPerCluster           db 0
ReservedSectors             dw 0
FatCount                    db 2
RootDirectoryEntriesCount   dw 0
TotalSectors16              dw 0
MediaDescriptor             db 0f8h
SectorsPerFat16             dw 0

dos_3_31_bpb:
SectorsPerTrack             dw 0
NumberOfHeads               dw 0
HiddenSectors               dd 0
TotalSectors32              dd 0

dos_7_01_ebpb:
SectorsPerFat32             dd 0
MirroringFlags              dw 0
FatVersion                  dw 0
RootDirectoryCluster        dd 2
FileSystemInfoSector        dw 1
BackupSectors               dw 6
TIMES 12                    db 0
BootDisk                    db 80h
NtFlags                     db 0
ExtendedBootSignature       db 29h
VolumeSerialNumber          dd 0
VolumeLabel                 db "NO NAME    "
SystemIdentifierString      db "FAT32   "

TIMES 90 - ($ - $$)         db ' '

ALIGN   4

InitLoad:

    ;
    ;   in 16 bit mode, the calling convention is to use the smallest op's as possible
    ;   without corrupting registers. we can use pusha/d for saving 
    ;   nothing is passed by the stack other than return addresses really.
    ;

    xor     ax, ax
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax
    mov     esp, 7c00h
    mov     ebp, esp

    mov     byte [BootDisk], dl
    
    jmp     0x00:Load

%INCLUDE    "bootdisk.asm"

;
;   we're hardcoding directories.
;   and files for now.
;
;   LdrBootInfo is a small structure of information
;   the boot loader needs passing to the bootmgr.
;

%DEFINE LdrBootInfo         0x2000
%DEFINE PageMapBase         0x10000

;   struct: MEMORY_MAP_ENTRY
;   ULONG64 RegionBase
;   ULONG64 RegionLength
;   ULONG32 RegionType
;   ULONG32 AcpiEa
;

;   struct:             MEMORY_MAP
;   ULONG32             EntryCount
;   MEMORY_MAP_ENTRY    Entries[ 0 ]
;

;
;   struct: RD_FILE_LIST_ENTRY (20)
;   CHAR    DirectoryFile[ 12 ]
;   ULONG32 FilePointer // Segment:Offset
;   ULONG32 Length
;

;
;   struct: RD_FILE_LIST
;   ULONG32             RdFileCount
;   RD_FILE_LIST_ENTRY  RdEntries[ 0 ]
;

;   struct:         LDR_BOOT_INFO
;   ULONG32         DisplayWidth
;   ULONG32         DisplayHeight
;   ULONG32         BitsPerPixel
;   ULONG32         Framebuffer
; N ULONG32         RootSerial
;   PMEMORY_MAP     MemoryMap // ULONG32
;   PRD_FILE_LIST   FileList // ULONG32
;

BootSystemDirectory         db "SYSTEM     " ; this is the system root directory
BootInitrdDirectory         db "BOOT       " ; this is the initrd directory passed to the kernel
BootKernelFile              db "CARBKRNLSYS" ; the kernel, expected to be in the system root directory, not initrd.

SystemDirectoryBase         dd 20000000h
InitrdDirectoryBase         dd 0
KernelFileBase              dd 0
InitrdFileReadPointer       dd 0

ALIGN   4

Load:
    
    in      al, 92h
    or      al, 02h
    out     92h, al

    mov     al, 0ffh
    out     0a1h, al
    out     021h, al
    cli

    ;
    ;   load the following sectors of the boot loader
    ;   directly after this
    ;

    movzx   eax, word [BackupSectors]
    add     eax, dword [HiddenSectors]
    add     eax, 3
    xor     edx, edx
    mov     cx, 3
    mov     ebx, 7e00h
    call    BootDiskReadSectors

    ;
    ;   read the root directory and prepare
    ;   to setup the initrd
    ;

    mov     edx, 2
    mov     ebx, dword [SystemDirectoryBase]
    call    BootFsFat32ReadCluster

    mov     ebx, dword [SystemDirectoryBase]
    mov     eax, ebx
    mov     cx, BootSystemDirectory
    call    BootFsFat32ReadDirectoryFile

    mov     dword [InitrdDirectoryBase], ebx
    mov     cx, BootInitrdDirectory
    call    BootFsFat32ReadDirectoryFile

    mov     dword [KernelFileBase], ebx
    mov     cx, BootKernelFile
    call    BootFsFat32ReadDirectoryFile
    mov     dword [InitrdFileReadPointer], ebx

    mov     eax, dword [InitrdDirectoryBase]
    mov     ecx, LdrBootInfo + 32
    call    BootFsFat32ReadRdDirectory
    mov     dword [LdrBootInfo + 24], LdrBootInfo + 32

    mov     eax, LdrBootInfo
    call    VesaSetMode

    mov     eax, dword [VolumeSerialNumber]
    mov     dword [LdrBootInfo + 16], eax
    jmp     Load1

%ASSIGN     BYTES_LEFT 510 - ($ - $$)
%WARNING    SECTOR 0 HAS BYTES_LEFT BYTES LEFT.
TIMES       BYTES_LEFT db   0
dw                          0xAA55

MemoryMapInitialize:

    ;
    ;   calculate the next free chunk of memory
    ;   after the initrd list and setup a memory map
    ;   using e820.
    ;

    pushad
    mov     eax, 20
    mov     ecx, dword [LdrBootInfo + 32]
    mul     ecx
    lea     edi, [LdrBootInfo + eax + 36]
    mov     dword [LdrBootInfo + 20], edi
    mov     ebp, edi
    add     edi, 4
    mov     dword [ebp], 0

    xor     ebx, ebx
    mov     edx, 'PAMS'
    mov     eax, 0e820h
    mov     ecx, 24
    int     15h
    jc      .MemoryMapFailure

    cmp     eax, 'PAMS'
    jne     .MemoryMapFailure

    test    ebx, ebx
    jz      .MemoryMapFailure
    jmp     .MemoryMapEnumerate

.MemoryMapQueryEntry:
    mov     edx, 'PAMS'
    mov     ecx, 24
    mov     eax, 0e820h
    int     15h

.MemoryMapEnumerate:
    jcxz    .MemoryMapNextEntry
    mov     ecx, [di+8]
    or      ecx, [di+12]
    jz      .MemoryMapNextEntry

    inc     dword [ebp]
    add     di, 24
.MemoryMapNextEntry:
    test    ebx, ebx
    jnz     .MemoryMapQueryEntry
    jmp     .MemoryMapSuccess

.MemoryMapFailure:
    mov     eax, 0D3ADB00Bh
    cli
    hlt
.MemoryMapSuccess:
    popad
    ret


PageMapFillTable:
    ;
    ;   EBX     = PhysicalMapping (with page table attributes)
    ;   ES:DI   = Segment:offset for page table 
    ;   this is just a simple loop that has been converted to a 
    ;   function for convineince
    ;

    pushad
    xor     ecx, ecx

.PageMapEntry:
    mov     dword es:[di], ebx
    add     ebx, 1000h
    add     di, 8
    inc     ecx
    cmp     ecx, 200h
    jne     .PageMapEntry

    popad
    ret

PageMapInitialize:
    pushad
    mov     eax, es
    push    eax

    mov     edi, PageMapBase
    mov     cr3, edi
    
    xor     di, di
    mov     ax, PageMapBase >> 4
    mov     es, ax

    xor     ax, ax
    mov     cx, 6000h

    cld
    rep     stosb

    xor     di, di
    mov     dword es:[di], PageMapBase + 1000h * 1 + 3
    add     di, 1000h
    mov     dword es:[di], PageMapBase + 1000h * 2 + 3
    add     di, 1000h
    mov     dword es:[di], PageMapBase + 1000h * 3 + 3
    add     di, 1000h

    mov     ebx, 3
    call    PageMapFillTable

    mov     di, 0ff8h
    mov     dword es:[di], PageMapBase + 1000h * 4 + 3
    mov     di, 4ff8h
    mov     dword es:[di], PageMapBase + 1000h * 5 + 3
    add     di, 1000h
    mov     dword es:[di], PageMapBase + 1000h * 6 + 3
    mov     di, 6000h

    mov     ebx, 200003h
    call    PageMapFillTable

    mov     di, 0ff0h
    mov     dword es:[di], PageMapBase + 3

    pop     eax
    mov     es, eax
    popad
    ret

Load1:

    call MemoryMapInitialize
    call PageMapInitialize

    mov     eax, cr4
    or      eax, 1 << 5 | 3 << 9
    mov     cr4, eax

    ;
    ;   copy the global descriptor table to 1800h
    ;   it is placed here because we dont want to
    ;   overwrite any bda at (0 - 1000h) and 1000h
    ;   is reserved for SIPI's.
    ;

    mov     si, GlobalDescriptorTable
    mov     di, 1800h
    mov     cx, 24
    rep     movsb

    mov     word es:[di], 23
    mov     dword es:[di+2], 1800h

    lgdt    es:[di]

    mov     ecx, 0c0000080h
    rdmsr
    or      eax, 1 << 8
    wrmsr

    mov     eax, cr0
    or      eax, 1 << 31 | 1 << 0;1 << 31 | 1 << 0 | 1 << 1 
    ; and     eax, ~(1 << 2 | 2 << 29)
    mov     cr0, eax

    mov     ax, 16
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax

    jmp     8:Load2

BITS    64

Load2:
    mov     rsp, 10000h
    mov     rax, cr0
    or      rax, 1 << 1
    and     rax, ~(1 << 2 | 3 << 29)
    mov     cr0, rax
    mov     ecx, dword [KernelFileBase]
    shr     rcx, 12
    add     cx, word [KernelFileBase]
    sub     rsp, 40
    call    ImageLdrLoadFile
    
    mov     rcx, LdrBootInfo
    jmp     rax
    cli
    hlt

%ASSIGN     BYTES_LEFT 1022 - ($ - $$)
%WARNING    SECTOR 1 HAS BYTES_LEFT BYTES LEFT.
TIMES       BYTES_LEFT db   0
dw                          0xAA55

GlobalDescriptorTable:
    dq      0

    dw      0
    dw      0
    db      0
    db      10011010b
    db      10101111b
    db      0

    dw      0
    dw      0
    db      0
    db      10010010b
    db      0
    db      0

%INCLUDE    "fat32read.asm"
%INCLUDE    "vesa.asm"
%INCLUDE    "peldr.asm"

%ASSIGN     BYTES_LEFT 2046 - ($ - $$)
%WARNING    SECTOR 2 HAS BYTES_LEFT BYTES LEFT.
TIMES       BYTES_LEFT db   0
dw                          0xAA55