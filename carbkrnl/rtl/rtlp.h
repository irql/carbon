


#pragma once

#define RtlTrapFrameToContext( TrapFrame, Context ) \
( Context )->SegCs = ( TrapFrame )->SegCs; \
( Context )->SegSs = ( TrapFrame )->SegSs; \
( Context )->SegDs = ( TrapFrame )->SegDs; \
( Context )->SegEs = ( TrapFrame )->SegEs; \
( Context )->SegFs = ( TrapFrame )->SegFs; \
( Context )->SegGs = ( TrapFrame )->SegGs; \
( Context )->EFlags = ( TrapFrame )->EFlags; \
( Context )->Rax = ( TrapFrame )->Rax; \
( Context )->Rcx = ( TrapFrame )->Rcx; \
( Context )->Rdx = ( TrapFrame )->Rdx; \
( Context )->Rbx = ( TrapFrame )->Rbx; \
( Context )->Rsp = ( TrapFrame )->Rsp; \
( Context )->Rbp = ( TrapFrame )->Rbp; \
( Context )->Rsi = ( TrapFrame )->Rsi; \
( Context )->Rdi = ( TrapFrame )->Rdi; \
( Context )->R8 = ( TrapFrame )->R8; \
( Context )->R9 = ( TrapFrame )->R9; \
( Context )->R10 = ( TrapFrame )->R10; \
( Context )->R11 = ( TrapFrame )->R11; \
( Context )->R12 = ( TrapFrame )->R12; \
( Context )->R13 = ( TrapFrame )->R13; \
( Context )->R14 = ( TrapFrame )->R14; \
( Context )->R15 = ( TrapFrame )->R15; \
( Context )->Rip = ( TrapFrame )->Rip; \
( Context )->Dr0 = ( TrapFrame )->Dr0; \
( Context )->Dr1 = ( TrapFrame )->Dr1; \
( Context )->Dr2 = ( TrapFrame )->Dr2; \
( Context )->Dr3 = ( TrapFrame )->Dr3; \
( Context )->Dr6 = ( TrapFrame )->Dr6; \
( Context )->Dr7 = ( TrapFrame )->Dr7


#define RtlContextToTrapFrame( TrapFrame, Context ) \
( TrapFrame )->SegCs = ( Context )->SegCs; \
( TrapFrame )->SegSs = ( Context )->SegSs; \
( TrapFrame )->SegDs = ( Context )->SegDs; \
( TrapFrame )->SegEs = ( Context )->SegEs; \
( TrapFrame )->SegFs = ( Context )->SegFs; \
( TrapFrame )->SegGs = ( Context )->SegGs; \
( TrapFrame )->EFlags = ( Context )->EFlags; \
( TrapFrame )->Rax = ( Context )->Rax; \
( TrapFrame )->Rcx = ( Context )->Rcx; \
( TrapFrame )->Rdx = ( Context )->Rdx; \
( TrapFrame )->Rbx = ( Context )->Rbx; \
( TrapFrame )->Rsp = ( Context )->Rsp; \
( TrapFrame )->Rbp = ( Context )->Rbp; \
( TrapFrame )->Rsi = ( Context )->Rsi; \
( TrapFrame )->Rdi = ( Context )->Rdi; \
( TrapFrame )->R8 = ( Context )->R8; \
( TrapFrame )->R9 = ( Context )->R9; \
( TrapFrame )->R10 = ( Context )->R10; \
( TrapFrame )->R11 = ( Context )->R11; \
( TrapFrame )->R12 = ( Context )->R12; \
( TrapFrame )->R13 = ( Context )->R13; \
( TrapFrame )->R14 = ( Context )->R14; \
( TrapFrame )->R15 = ( Context )->R15; \
( TrapFrame )->Rip = ( Context )->Rip; \
( TrapFrame )->Dr0 = ( Context )->Dr0; \
( TrapFrame )->Dr1 = ( Context )->Dr1; \
( TrapFrame )->Dr2 = ( Context )->Dr2; \
( TrapFrame )->Dr3 = ( Context )->Dr3; \
( TrapFrame )->Dr6 = ( Context )->Dr6; \
( TrapFrame )->Dr7 = ( Context )->Dr7

BOOLEAN
RtlpTrapAssertionFailure(
    _In_ PKINTERRUPT Interrupt
);

NTSTATUS
RtlpUnwindPrologue(
    _In_ PKTHREAD                Thread,
    _In_ PCONTEXT                TargetContext,
    _In_ PVOID                   TargetBase,
    _In_ PIMAGE_RUNTIME_FUNCTION TargetFunction,
    _In_ ULONG64                 FrameBase
);

NTSTATUS
RtlpUnwindPrologueFrame(
    _In_  PKTHREAD                Thread,
    _In_  PCONTEXT                TargetContext,
    _In_  PVOID                   TargetBase,
    _In_  PIMAGE_RUNTIME_FUNCTION TargetFunction,
    _In_  ULONG64                 StackBase,
    _In_  ULONG64                 StackLength,
    _Out_ PULONG64                FrameBase
);

NTSTATUS
RtlpFindExceptionHandler(
    _In_  PEXCEPTION_RECORD   Record,
    _In_  PMM_VAD             Vad,
    _Out_ PEXCEPTION_HANDLER* Handler,
    _Out_ PUNWIND_INFO*       Unwind
);

BOOLEAN
RtlpHandleHighIrqlException(
    _In_ PEXCEPTION_RECORD Record,
    _In_ PUNWIND_INFO      Unwind,
    _In_ PKTRAP_FRAME      TrapFrame
);
