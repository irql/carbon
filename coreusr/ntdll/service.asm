


BITS    64
SECTION .text

%MACRO  SYSTEM_SERVICE 2
GLOBAL  %1
EXPORT  %1
%1:
    mov     r10, rcx
    mov     eax, %2
    syscall 
    ret
%ENDMACRO

SYSTEM_SERVICE NtCreateFile, 0
SYSTEM_SERVICE NtReadFile, 1
SYSTEM_SERVICE NtDeviceIoControlFile, 2
SYSTEM_SERVICE NtCreateSection, 3
SYSTEM_SERVICE NtMapViewOfSection, 4
SYSTEM_SERVICE NtUnmapViewOfSection, 5
SYSTEM_SERVICE NtResizeSection, 6
SYSTEM_SERVICE NtClose, 7
SYSTEM_SERVICE NtCreateThread, 8
SYSTEM_SERVICE RtlDebugPrint, 9
SYSTEM_SERVICE NtWaitForSingleObject, 10
SYSTEM_SERVICE NtAllocateVirtualMemory, 11
SYSTEM_SERVICE NtFreeVirtualMemory, 12
SYSTEM_SERVICE NtQuerySystemClock, 13
SYSTEM_SERVICE NtCreateMutex, 14
SYSTEM_SERVICE NtReleaseMutex, 15
SYSTEM_SERVICE NtGetTickCount, 16
SYSTEM_SERVICE NtQueryInformationFile, 17
SYSTEM_SERVICE NtQueryDirectoryFile, 18
SYSTEM_SERVICE NtWriteFile, 19

