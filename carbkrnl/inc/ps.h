


#pragma once

NTSYSAPI EXTERN POBJECT_TYPE PsThreadObject;
NTSYSAPI EXTERN POBJECT_TYPE PsProcessObject;

NTSYSAPI
PKTHREAD
PsGetCurrentThread(

);

NTSYSAPI
PKPROCESS
PsGetCurrentProcess(

);

NTSYSAPI
ULONG64
PsGetThreadId(
    _In_ PKTHREAD Thread
);

#define THREAD_SYSTEM       0x01
#define THREAD_SUSPENDED    0x02

NTSYSAPI EXTERN PKPROCESS PsInitialSystemProcess;

NTSYSAPI
NTSTATUS
ZwCreateThread(
    _Out_     PHANDLE            ThreadHandle,
    _In_      HANDLE             ProcessHandle,
    _In_      ACCESS_MASK        DesiredAccess,
    _In_      PKSTART_ROUTINE    ThreadStart,
    _In_      PVOID              ThreadContext,
    _In_      ULONG32            Flags,
    _In_      POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_  ULONG64            StackLength,
    _Out_opt_ PULONG64           ThreadId
);

NTSYSAPI
NTSTATUS
ZwCreateSystemThread(
    _Out_ PHANDLE         ThreadHandle,
    _In_  ACCESS_MASK     DesiredAccess,
    _In_  PKSTART_ROUTINE ThreadStart,
    _In_  PVOID           ThreadContext,
    _In_  ULONG32         Flags,
    _In_  ULONG64         StackLength
);

NTSYSAPI
NTSTATUS
ZwTerminateThread(
    _In_ HANDLE   ThreadHandle,
    _In_ NTSTATUS ExitCode
);

NTSYSAPI
NTSTATUS
ZwSuspendThread(
    _In_      HANDLE   ThreadHandle,
    _Out_opt_ PULONG64 SuspendCount
);

NTSYSAPI
NTSTATUS
ZwResumeThread(
    _In_      HANDLE   ThreadHandle,
    _Out_opt_ PULONG64 SuspendCount
);

NTSYSAPI
NTSTATUS
ZwSetContextThread(
    _In_ HANDLE   ThreadHandle,
    _In_ PCONTEXT Context
);

NTSYSAPI
NTSTATUS
ZwGetContextThread(
    _In_ HANDLE   ThreadHandle,
    _In_ PCONTEXT Context
);

NTSYSAPI
NORETURN
VOID
ZwContinue(
    _In_ PCONTEXT Context
);

NTSYSAPI
NTSTATUS
ZwOpenThread(
    _Out_ PHANDLE            ThreadHandle,
    _In_  ACCESS_MASK        DesiredAccess,
    _In_  POBJECT_ATTRIBUTES ObjectAttributes,
    _In_  ULONG64            ThreadId
);

NTSYSAPI
KPROCESSOR_MODE
PsGetPreviousMode(
    _In_ PKTHREAD Thread
);

NTSYSAPI
PKPROCESS
PsGetThreadProcess(
    _In_ PKTHREAD Thread
);

NTSYSAPI
VOID
PsSuspendThread(
    _In_      PKTHREAD Thread,
    _Out_opt_ PULONG64 SuspendCount
);

NTSYSAPI
VOID
PsResumeThread(
    _In_      PKTHREAD Thread,
    _Out_opt_ PULONG64 SuspendCount
);

NTSYSAPI
VOID
PsSetThreadProcessor(
    _In_ PKTHREAD Thread,
    _In_ ULONG64  ProcessorNumber
);

NTSTATUS
NtCreateThread(
    _Out_     PHANDLE            ThreadHandle,
    _In_      HANDLE             ProcessHandle,
    _In_      ACCESS_MASK        DesiredAccess,
    _In_      PKSTART_ROUTINE    ThreadStart,
    _In_      PVOID              ThreadContext,
    _In_      ULONG32            Flags,
    _In_      POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_  ULONG64            StackLength,
    _Out_opt_ PULONG64           ThreadId
);
