


#include <carbsup.h>

typedef struct _X86_BIOS_REGISTERS {
    ULONG32 Eax;
    ULONG32 Ecx;
    ULONG32 Edx;
    ULONG32 Ebx;
    ULONG32 Ebp;
    ULONG32 Esi;
    ULONG32 Edi;
    USHORT  SegDs;
    USHORT  SegEs;

} X86_BIOS_REGISTERS, *PX86_BIOS_REGISTERS;
