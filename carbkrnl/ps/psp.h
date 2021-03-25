


#pragma once

#define PS_TAG '  SP'

VOID
PsInitializeProcessSystem(

);

typedef struct _PS_SYSTEM_STACK {

    //
    // Linked together on your process structure.
    //

    LIST_ENTRY      StackLinks;
    ULONG64         Address;
    ULONG64         Length;

} PS_SYSTEM_STACK, *PPS_SYSTEM_STACK;

PVOID
PspCreateStack(
    _In_ HANDLE  ProcessHandle,
    _In_ ULONG64 Length
);

VOID
PspDestroyStack(
    _In_ HANDLE  ProcessHandle,
    _In_ PVOID   Base,
    _In_ ULONG64 Length,
    _In_ BOOLEAN User
);

PPS_SYSTEM_STACK
PspQueryStack(
    _In_ PKPROCESS Process,
    _In_ PVOID     Pointer
);

VOID
PspSystemThreadReturn(

);

ULONG64
PspGetThreadProcessor(

);

VOID
PspCleanupThread(
    _In_ PKTHREAD Thread
);

VOID
PspCreateInitialUserProcess(

);

NTSTATUS
PspCreateUserProcess(
    _Out_ PHANDLE            ProcessHandle,
    _In_  ACCESS_MASK        DesiredAccess,
    _In_  POBJECT_ATTRIBUTES ObjectAttributes
);

VOID
PspLoadKnownDll(
    _Out_ PHANDLE            SectionHandle,
    _In_  POBJECT_ATTRIBUTES FileAttributes,
    _In_  POBJECT_ATTRIBUTES SectionAttributes
);

NTSTATUS
PspCreateUserProcess(
    _Out_ PHANDLE            ProcessHandle,
    _In_  ACCESS_MASK        DesiredAccess,
    _In_  POBJECT_ATTRIBUTES ObjectAttributes
);
