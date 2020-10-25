

;;;struct VBE_MODE_INFO {
;
;
;;;};

;writes info to the buffer point to by
;VBE_MODE_INFO

;ax = width
;bx = height
;cl = bpp

%define VIB_VIDEO_MODES		14

%define VMI_WIDTH			18
%define VMI_HEIGHT			20
%define VMI_BPP				25
%define VMI_FRAMEBUFFER		40

VbeSetMode:
	pushad

	mov dx, VBE_MODE_INFO >> 4
	mov fs, dx

	mov word [fs:0], ax
	mov word [fs:2], bx
	mov byte [fs:4], cl
	
	mov ax, 0x4f00
	mov di, 0x2000
	int 0x10

	cmp ax, 0x4f
	jne .VbeError

	mov ax, word [0x2000+VIB_VIDEO_MODES]
	mov si, ax
	mov ax, word [0x2000+VIB_VIDEO_MODES+2]
	mov gs, ax

.VbeFind:
	mov cx, word [gs:si]
	add si, 2

	cmp cx, 0xffff
	je .VbeError

	mov ax, 0x4f01
	mov di, 0x4000
	int 0x10

	cmp ax, 0x4f
	jne .VbeError

	mov ax, word [fs:0]
	cmp word [0x4000+VMI_WIDTH], ax
	jne .VbeFind

	mov ax, word [fs:2]
	cmp word [0x4000+VMI_HEIGHT], ax
	jne .VbeFind

	mov al, byte [fs:4]
	cmp byte [0x4000+VMI_BPP], al
	jne .VbeFind

	mov eax, dword [0x4000+VMI_FRAMEBUFFER]
	mov dword [fs:5], eax

	mov ax, 0x4f02
	mov bx, cx
	or bx, 0x4000
	xor di, di
	int 0x10

	cmp ax, 0x4f
	jne .VbeError
	
	popad
	xor ax, ax
	mov ds, ax
	mov gs, ax
	ret

.VbeError:
	;mov eax, 0xdeadbab0

	cli
	hlt


	


