

bits 64

%define sizeof_FileHeader			0x14
%define sizeof_SectionHeader		0x28
%define sizeof_DataDirectory		0x8
%define sizeof_BaseReloc			0x8

%define dos_e_lfanew				0x3c

%define nt_FileHeader				0x4
%define nt_OptionalHeader			nt_FileHeader+sizeof_FileHeader

%define fh_SizeOfOptionalHeader		0x10
%define fh_NumberOfSections			0x2

%define oh_AddressOfEntryPoint		0x10
%define oh_ImageBase				0x18
%define oh_SizeOfHeaders			0x3c
%define oh_DataDirectory			0x70

%define sh_VirtualSize				0x8
%define sh_VirtualAddress			0xc
%define sh_RawSize					0x10
%define sh_RawAddress				0x14
%define sh_Characteristics			0x24

%define br_VirtualAddress			0x0
%define br_SizeOfBlock				0x4

MapImage:

	;;
	;;	maps the image arg into memory 
	;;	and returns the entry point.
	;;

	;push rbp
	;mov rbp, rsp

	;rsi = dos header
	;r11 = nt header
	;r12 = section headers
	;r13 = size_of_headers
	;rdi = image_base

	mov rsi, rcx
	movsx r11, dword [rsi+dos_e_lfanew]
	movzx r12, word [rsi+r11+nt_FileHeader+fh_SizeOfOptionalHeader]
	add r12, nt_OptionalHeader
	add r12, r11
	movsx r13, dword [rsi+r11+nt_OptionalHeader+oh_SizeOfHeaders]
	mov rdi, 0xffff800000000000;qword [rsi+r11+nt_OptionalHeader+oh_ImageBase]

	;copy pe headers to image base
	push rdi
	push rsi
	mov rcx, r13
	cld
	rep movsb
	pop rsi
	pop rdi

	movzx r14, word [rsi+r11+nt_FileHeader+fh_NumberOfSections]
	xor rcx, rcx
	.MapSection:
	push rcx
	imul rcx, sizeof_SectionHeader

	lea rax, [rsi+r12]
	add rax, rcx

	movsx r15, dword [rax+sh_VirtualAddress]
	movsx rbx, dword [rax+sh_RawAddress]
	;movsx rcx, dword [rax+sh_VirtualSize] BAD

	;movsx rcx, dword [rax+sh_RawSize]

	push rdi
	push rsi
	add rdi, r15
	add rsi, rbx
	;rdi = section va
	;rcx = section vs
	;rsi = raw address
	cld

	;zero the entire section.
	movsx rcx, dword [rax+sh_VirtualSize]
	push rdi
	push rax
	xor rax, rax
	rep stosb
	pop rax
	pop rdi
	movsx rcx, dword [rax+sh_RawSize]

	
	rep movsb
	pop rsi
	pop rdi

	pop rcx
	inc rcx
	cmp rcx, r14
	jne MapImage.MapSection
	
	;;rsi = dos
	;;rdi = base
	mov r14, rdi
	sub r14, qword [rsi+r11+nt_OptionalHeader+oh_ImageBase]
	;r14 = delta.
	;r15 = base reloc

	movsx r15, dword [rsi+r11+nt_OptionalHeader+oh_DataDirectory+(5*sizeof_DataDirectory)]
	add r15, rdi

	push r11
	.BaseReloc:

	cmp dword [r15+br_VirtualAddress], 0
	je MapImage.BaseRelocDone

	movsx rcx, dword [r15+br_SizeOfBlock]

	cmp ecx, sizeof_BaseReloc
	jle MapImage.BaseRelocNext

	sub rcx, sizeof_BaseReloc

	lea r13, [r15+sizeof_BaseReloc]

	.NextReloc:
	cmp word [r13], 0
	je .N

	movzx r11, word [r13]
	and r11d, 0xfff
	add r11d, dword [r15+br_VirtualAddress]
	;r11 = BaseReloc->VirtualAddress + (RelocList[i] & 0xfff);

	lea r12, [rdi+r11]
	add qword [r12], r14

	.N:
	add r13, 2
	sub rcx, 2
	jz MapImage.BaseRelocNext
	jmp MapImage.NextReloc

	.BaseRelocNext:
	
	movsx r13, dword [r15+br_SizeOfBlock] 
	add r15, r13
	jmp MapImage.BaseReloc
	

	.BaseRelocDone:
	pop r11
	movsx rax, dword [rsi+r11+nt_OptionalHeader+oh_AddressOfEntryPoint]
	add rax, rdi

	;mov rsp, rbp
	;pop rbp
	ret
	

