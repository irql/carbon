


bits 64

section .text

%macro DECLARE_SYSTEM_SERVICE 2
global %1
export %1
%1:
	mov r10, rcx
	mov eax, %2
	syscall
	ret
%endmacro

DECLARE_SYSTEM_SERVICE NtCreateFile, 0
DECLARE_SYSTEM_SERVICE NtReadFile, 1
DECLARE_SYSTEM_SERVICE NtWriteFile, 2 
DECLARE_SYSTEM_SERVICE NtClose, 3 
DECLARE_SYSTEM_SERVICE NtQueryDirectoryFile, 4 
DECLARE_SYSTEM_SERVICE NtQueryInformationFile, 5 
DECLARE_SYSTEM_SERVICE NtSetInformationFile, 6 
DECLARE_SYSTEM_SERVICE NtDeviceIoControlFile, 7 
DECLARE_SYSTEM_SERVICE NtDisplayString, 8 