


#pragma once


typedef enum _POOL_TYPE {
    UnusedPool,
    NonPagedPool,
    NonPagedPoolExecute,
    NonPagedPoolZeroed,
    NonPagedPoolZeroedExecute,
    MaximumPool

} POOL_TYPE;

typedef enum _MM_CACHE_TYPE {
    MmCacheWriteBack,       // Pa0
    MmCacheWriteCombining,  // Pa1
    MmCacheUncacheable,     // Pa2
    MmCacheMaximum
} MM_CACHE_TYPE;

NTSYSAPI
PVOID
MmMapIoSpace(
    _In_ ULONG64 Physical,
    _In_ ULONG64 Length
);

NTSYSAPI
PVOID
MmMapIoSpaceSpecifyCache(
    _In_ ULONG64       Physical,
    _In_ ULONG64       Length,
    _In_ MM_CACHE_TYPE Cache
);

NTSYSAPI
PVOID
MmAllocatePoolWithTag(
    _In_ POOL_TYPE Type,
    _In_ ULONG64   Length,
    _In_ ULONG32   Tag
);

NTSYSAPI
VOID
MmFreePoolWithTag(
    _In_ PVOID   Pool,
    _In_ ULONG32 Tag
);

NTSYSAPI
VOID
MmUnmapIoSpace(
    _In_ PVOID Pool
);

NTSYSAPI EXTERN POBJECT_TYPE MmSectionObject;

NTSYSAPI
NTSTATUS
MmCreateSection(
    _Out_    PMM_SECTION_OBJECT* SectionObject,
    _In_opt_ POBJECT_ATTRIBUTES  ObjectAttributes,
    _In_     ULONG               AllocationAttributes,
    _In_opt_ PIO_FILE_OBJECT     FileObject
);

NTSYSAPI
NTSTATUS
MmCreateSectionSpecifyAddress(
    _Out_     PMM_SECTION_OBJECT* SectionObject,
    _In_opt_  POBJECT_ATTRIBUTES  ObjectAttributes,
    _In_      ULONG               AllocationAttributes,
    _In_      ULONG64             Address,
    _In_      ULONG64             Length
);

NTSYSAPI
NTSTATUS
MmResizeSection(
    _In_ PMM_SECTION_OBJECT SectionObject,
    _In_ ULONG64            SectionLength
);

NTSYSAPI
NTSTATUS
MmMapViewOfSection(
    _In_    PMM_SECTION_OBJECT SectionObject,
    _In_    PKPROCESS          Process,
    _Inout_ PVOID*             BaseAddress,
    _In_    ULONG64            ViewOffset,
    _In_    ULONG64            ViewLength,
    _In_    ULONG              Protection
);

NTSYSAPI
NTSTATUS
MmUnmapViewOfSection(
    _In_ PKPROCESS Process,
    _In_ PVOID     BaseAddress
);

NTSYSAPI
NTSTATUS
ZwCreateSection(
    _Out_    PHANDLE            SectionHandle,
    _In_     ACCESS_MASK        DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_     ULONG              AllocationAttributes,
    _In_opt_ HANDLE             FileHandle
);

NTSYSAPI
NTSTATUS
ZwResizeSection(
    _In_ HANDLE  SectionHandle,
    _In_ ULONG64 SectionLength
);

NTSYSAPI
NTSTATUS
ZwMapViewOfSection(
    _In_  HANDLE  SectionHandle,
    _In_  HANDLE  ProcessHandle,
    _Out_ PVOID*  BaseAddress,
    _In_  ULONG64 ViewOffset,
    _In_  ULONG64 ViewLength,
    _In_  ULONG   Protection
);

NTSYSAPI
NTSTATUS
ZwUnmapViewOfSection(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID  BaseAddress
);

NTSYSAPI
NTSTATUS
ZwAllocateVirtualMemory(
    _In_    HANDLE  ProcessHandle,
    _Inout_ PVOID*  BaseAddress,
    _In_    ULONG64 Length,
    _In_    ULONG32 Protect
);

NTSYSAPI
NTSTATUS
ZwFreeVirtualMemory(
    _In_ HANDLE  ProcessHandle,
    _In_ PVOID   BaseAddress,
    _In_ ULONG64 Length
);

NTSTATUS
NtCreateSection(
    _Out_    PHANDLE            SectionHandle,
    _In_     ACCESS_MASK        DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_     ULONG              AllocationAttributes,
    _In_opt_ HANDLE             FileHandle
);

NTSTATUS
NtMapViewOfSection(
    _In_  HANDLE  SectionHandle,
    _In_  HANDLE  ProcessHandle,
    _Out_ PVOID*  BaseAddress,
    _In_  ULONG64 ViewOffset,
    _In_  ULONG64 ViewLength,
    _In_  ULONG   Protection
);

NTSTATUS
NtUnmapViewOfSection(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID  BaseAddress
);

NTSTATUS
NtResizeSection(
    _In_ HANDLE  SectionHandle,
    _In_ ULONG64 SectionLength
);

NTSTATUS
NtAllocateVirtualMemory(
    _In_    HANDLE  ProcessHandle,
    _Inout_ PVOID*  BaseAddress,
    _In_    ULONG64 Length,
    _In_    ULONG32 Protect
);

NTSTATUS
NtFreeVirtualMemory(
    _In_ HANDLE  ProcessHandle,
    _In_ PVOID   BaseAddress,
    _In_ ULONG64 Length
);
