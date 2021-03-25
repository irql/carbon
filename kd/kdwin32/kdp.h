


#pragma once

PKPROCESS
KdpGetThreadProcess(
    _In_ PKTHREAD Thread
);

BOOLEAN
KdpFindVadByAddress(
    _In_    PKPROCESS Process,
    _In_    ULONG64   Address,
    _Inout_ PMM_VAD   Vad
);

BOOLEAN
KdpFindVadByShortName(
    _In_    PKPROCESS Process,
    _In_    PWCHAR    ShortName,
    _Inout_ PMM_VAD   Vad
);

VOID
KdpGetVadFileName(
    _In_      PMM_VAD Vad,
    _Out_     PVOID   Buffer,
    _Out_opt_ PUSHORT RequiredLength
);

VOID
KdpHandleGuestException(
    _In_ PKD_PACKET Exception
);

NTSTATUS
KdpUnwindFrame(
    _In_ PKTHREAD Thread,
    _In_ PCONTEXT TargetContext
);

VOID
KdpGetThreadStack(
    _In_      PKTHREAD Thread,
    _Out_opt_ PULONG64 StackBase,
    _Out_opt_ PULONG64 StackLength
);

VOID
KdpGetThreadStackEx(
    _In_      PKTHREAD Thread,
    _Out_opt_ PULONG64 StackBase,
    _Out_opt_ PULONG64 StackLength,
    _Out_opt_ PULONG64 KernelStackBase,
    _Out_opt_ PULONG64 KernelStackLength
);

BOOLEAN
KdpReadDataDirectory(
    _In_      ULONG64  ModuleStart,
    _In_      ULONG64  Directory,
    _In_opt_  PVOID    Buffer,
    _Out_opt_ PULONG64 RequiredLength
);

typedef VOID( *PKD_HANDLE_COMMAND )(
    _In_ PKD_COMMAND Command
    );

VOID
KdpHandleContinue(
    _In_ PKD_COMMAND Command
);

VOID
KdpHandleDisplay(
    _In_ PKD_COMMAND Command
);

VOID
KdpHandleEdit(
    _In_ PKD_COMMAND Command
);

VOID
KdpHandleList(
    _In_ PKD_COMMAND Command
);

VOID
KdpHandleProcess(
    _In_ PKD_COMMAND Command
);

VOID
KdpHandleTraceStack(
    _In_ PKD_COMMAND Command
);

VOID
KdpHandleWsl(
    _In_ PKD_COMMAND Command
);

VOID
KdpBreakIn(

);

VOID
KdpContinue(

);
