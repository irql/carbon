bits 16
org 0x7c00

jmp short bldr
nop

;;these are just pointers to the real values, dont copy these to your disk.

dos_2_00_bpb:
FileSystemIdentifier        db "MSDOS5.0"
BytesPerSector              dw 512
SectorsPerCluster           db 32
ReservedSectors             dw 2250
FatCount                    db 2
RootDirectoryEntriesCount   dw 0
TotalSectors16              dw 0
MediaDescriptor             db 0xf8
SectorsPerFat16             dw 0

dos_3_31_bpb:
SectorsPerTrack             dw 63
NumberOfHeads               dw 255
HiddenSectors               dd 0
TotalSectors32              dd 62530624

dos_7_01_ebpb:
SectorsPerFat32             dd 15259
MirroringFlags              dw 0
FatVersion                  dw 0
RootDirectoryCluster        dd 2
FileSystemInfoSector        dw 1
BackupSectors               dw 6
times 12                    db 0
BootDisk                    db 0x80
NtFlags                     db 0
ExtendedBootSignature       db 0x29
VolumeSerialNumber          dd 0xBA4109EC
VolumeLabel                 db "NO NAME    "
SystemIdentifierString      db "FAT32   "

times 90 - ($ - $$) db 'B'

bldr:

xor ax, ax
mov ds, ax

mov es, ax
mov fs, ax
mov gs, ax
mov ss, ax

mov esp, 0x7c00
mov ebp, esp

mov byte [BootDisk], dl

jmp 0:Loader

%include "disk.asm"

%define TEMP_PML4T_BASE		0x10000
%define TEMP_GDT_BASE		0x500
%define MEMORY_MAP_BASE		0xC000
%define TEMP_KERNEL_STACK	0x10000
%define VBE_MODE_INFO		0xB000

%include "vbe.asm"

SystemFileName db "SYSTEM     "
KernelFileName db "KERNEL  SYS"
DiskFileName   db "DISK    SYS"

;;seg::offs
%define ROOT_DIRECTORY_BASE 	0x20000000
%define SYSTEM_DIRECTORY_BASE 	0x30000000
%define KERNEL_FILE_BASE 		0x40000000
%define DISK_FILE_BASE 			0x80000000

Loader:

	in al, 0x92
	or al, 2
	out 0x92, al

	movzx eax, word [BackupSectors]
	add eax, dword [HiddenSectors]
	add eax, 3
	xor edx, edx
	mov cx, 3
	mov ebx, 0x7e00
	call ReadSectors

	mov edx, 2
	mov ebx, ROOT_DIRECTORY_BASE
	call ReadCluster

	mov eax, ebx
	mov ebx, SYSTEM_DIRECTORY_BASE
	mov cx, SystemFileName
	call ReadDirectoryFile

	mov eax, ebx
	mov ebx, KERNEL_FILE_BASE
	mov cx, KernelFileName
	call ReadDirectoryFile

	mov eax, SYSTEM_DIRECTORY_BASE
	mov ebx, DISK_FILE_BASE
	mov cx, DiskFileName
	call ReadDirectoryFile

	mov ax, 1280
	mov bx, 720
	mov cl, 32
	call VbeSetMode

	jmp 0x7e00

%assign BYTES_LEFT 510 - ($ - $$)
%warning sector 0, BYTES_LEFT bytes left.

times BYTES_LEFT db 0
dw 0xaa55

s2:

	;;	kinda just snagged from some site.
	xor bp, bp
	mov di, MEMORY_MAP_BASE+4
	.InitializeE820MemoryMap:
	xor ebx, ebx

	mov edx, 'PAMS'
	mov eax, 0xe820
	mov ecx, 24
	int 0x15
	jc .E820Fail

	cmp eax, 'PAMS'
	jne .E820Fail
	cmp ebx, 0
	je .E820Fail
	jmp .E820Start

	.E820NextEntry:
	mov edx, 'PAMS'
	mov ecx, 24
	mov eax, 0xe820
	int 0x15
	.E820Start:
	jcxz .E820SkipEntry
	mov ecx, [es:di + 8]
	or ecx, [es:di + 12]
	jz .E820SkipEntry
	inc bp
	add di, 24
	.E820SkipEntry:
	cmp ebx, 0
	jne .E820NextEntry
	jmp .E820Done
	.E820Fail:
	cli
	hlt

	.E820Done:
	mov word [MEMORY_MAP_BASE], bp
	
	mov al, 0xff
	out 0xa1, al
	out 0x21, al
	cli

	.InitializePml4t:
	mov edi, TEMP_PML4T_BASE
	mov cr3, edi
	
	mov ax, 0x1000
	mov es, ax
	xor ax, ax
	mov cx, 0x6000

	cld
	rep stosb

	;mov edi, cr3
	xor di, di
	mov dword [es:di], TEMP_PML4T_BASE+0x1000*1+0x3
	add di, 0x1000
	mov dword [es:di], TEMP_PML4T_BASE+0x1000*2+0x3
	add di, 0x1000
	mov dword [es:di], TEMP_PML4T_BASE+0x1000*3+0x3
	add di, 0x1000
	;edi = pml4t[0]->pdpt[0]->pdt[0]->pt[0]

	mov ebx, 0x3
	call .FillTable
	jmp .cont

	.FillTable:
	xor ecx, ecx
	.FillTableLoop:
	mov dword [es:di], ebx
	add ebx, 0x1000
	add di, 8
	inc ecx
	cmp ecx, 0x200
	jne .FillTableLoop
	ret
	.cont:

	mov di, 0x800
	mov dword [es:di], TEMP_PML4T_BASE+0x1000*4+0x3
	mov di, 0x1000*4
	mov dword [es:di], TEMP_PML4T_BASE+0x1000*5+0x3
	add di, 0x1000
	mov dword [es:di], TEMP_PML4T_BASE+0x1000*6+0x3
	add di, 0x1000

	call .FillTable

	mov eax, cr4
	or eax, 1 << 5
	mov cr4, eax

	;;
	;;	move gdt to 0x500 and load.
	;;
	xor ax, ax
	mov es, ax
	mov ds, ax

    mov si, gdt
	mov di, TEMP_GDT_BASE
    mov cx, 8*3
    rep movsb

	mov word [es:di], 8*3-1
	mov dword [es:di+2], TEMP_GDT_BASE

	lgdt [es:di]

	;;
	;;	set LME bit.
	;;

	mov ecx, 0xc0000080
	rdmsr
	or eax, 1 << 8
	wrmsr

	;;
	;;	set PG and PM bits.
	;;

	mov eax, cr0
	or eax, 1 << 31 | 1 << 0
	mov cr0, eax

	;;
	;;	set segment selectors
	;;

	mov ax, 16
	mov ds, ax

	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	jmp 8:LongMode

gdt:
    dq 0

	dw 0
	dw 0
	db 0
	db 10011010b
	db 10101111b
	db 0

	dw 0
	dw 0
	db 0
	db 10010010b
	db 00000000b
	db 0


bits 64

LongMode:

	mov rsp, TEMP_KERNEL_STACK
	mov rbp, TEMP_KERNEL_STACK
	
	;no home addresses because i was retarded when i wrote this.
	mov rcx, (KERNEL_FILE_BASE >> 12) + (KERNEL_FILE_BASE & 0xFFFF);0x60000
	call MapImage
	
	sub rsp, 40
	mov rcx, MEMORY_MAP_BASE
	mov rdx, DISK_FILE_BASE
	mov r8, VBE_MODE_INFO
	jmp rax

%assign BYTES_LEFT 1022 - ($ - $$)
%warning sector 1, BYTES_LEFT bytes left.

times BYTES_LEFT db 0
dw 0xaa55

%include "fat32read.asm"

;si = errstr
ErrHandle:
    mov dl, byte [si]
    mov dh, 0x0f
    mov word [es:di], dx
    inc di
    inc di
    inc si
    cmp byte [si], 0
	jne ErrHandle

    ret

ErrNoLongMode db "Long mode not supported.", 0
ErrNotFound db " was not found.", 0

%assign BYTES_LEFT 1534 - ($ - $$)
%warning sector 2, BYTES_LEFT bytes left.

times BYTES_LEFT db 0
dw 0xaa55

%include "peldr.asm"

%assign BYTES_LEFT 2046 - ($ - $$)
%warning sector 3, BYTES_LEFT bytes left.

times BYTES_LEFT db 0
dw 0xaa55