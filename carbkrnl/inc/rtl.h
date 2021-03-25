


#pragma once

#define RtlUpperChar( c )               ( ( c ) >= 'a' && ( c ) <= 'z' ? ( c ) - ' ' : ( c ) )
#define RtlLowerChar( c )               ( ( c ) >= 'A' && ( c ) <= 'Z' ? ( c ) + ' ' : ( c ) )

#define RtlStringLength                 wcslen
#define RtlAnsiStringLength             strlen
#define RtlAnsiStringCompare            strcmp

NTSYSAPI
VOID
RtlZeroMemory(
    _In_ PVOID Destination,
    _In_ ULONG Length
);

NTSYSAPI
VOID
RtlCopyMemory(
    _In_ PVOID   Destination,
    _In_ PVOID   Source,
    _In_ ULONG64 Length
);

NTSYSAPI
VOID
RtlMoveMemory(
    _In_ PVOID Destination,
    _In_ PVOID Source,
    _In_ ULONG Length
);

NTSYSAPI
LONG
RtlCompareMemory(
    _In_ PVOID Memory1,
    _In_ PVOID Memory2,
    _In_ ULONG Length
);

NTSYSAPI
VOID
RtlFormatBufferFromArgumentList(
    _In_ PWSTR Buffer,
    _In_ PWSTR Format,
    _In_ VA_LIST List
);

NTSYSAPI
VOID
RtlIntToString(
    _In_ LONG  Value,
    _In_ PWSTR Buffer,
    _In_ ULONG Base
);

NTSYSAPI
LONG
RtlStringToInt(
    _In_ PWCHAR Buffer
);

NTSYSAPI
VOID
RtlStringReverse(
    _In_ PWSTR String
);

NTSYSAPI
VOID
RtlFormatBuffer(
    _In_ PWSTR Buffer,
    _In_ PWSTR Format,
    _In_ ...
);

NTSYSAPI
VOID
RtlDebugPrint(
    _In_ PWSTR Format,
    _In_ ...
);

NTSYSAPI
VOID
RtlDebugConsoleInit(
    _In_ ULONG32 Width,
    _In_ ULONG32 Height,
    _In_ ULONG32 Bpp,
    _In_ ULONG32 Fb
);

NTSYSAPI
VOID
RtlDebugScroll(
    _In_ LONG64 Scroll
);

NTSYSAPI
LONG
RtlCompareUnicodeString(
    _In_ PCUNICODE_STRING String1,
    _In_ PCUNICODE_STRING String2,
    _In_ BOOLEAN CaseInsensitive
);

NTSYSAPI
LONG
RtlCompareString(
    _In_ PCWSTR  String1,
    _In_ PCWSTR  String2,
    _In_ BOOLEAN CaseInsensitive
);

NTSYSAPI
LONG
RtlCompareStringLength(
    _In_ PCWSTR  String1,
    _In_ PCWSTR  String2,
    _In_ BOOLEAN CaseInsensitive,
    _In_ ULONG64 Length
);

NTSYSAPI
LONG
RtlCompareAnsiString(
    _In_ PCSTR   String1,
    _In_ PCSTR   String2,
    _In_ BOOLEAN CaseInsensitive
);

NTSYSAPI
LONG
RtlCompareAnsiStringLength(
    _In_ PCSTR   String1,
    _In_ PCSTR   String2,
    _In_ BOOLEAN CaseInsensitive,
    _In_ ULONG64 Length
);

FORCEINLINE
BOOLEAN
LdrVerifyDosHeader(
    _In_ PIMAGE_DOS_HEADER DosHeader
)
{
    return DosHeader->e_magic == IMAGE_DOS_SIGNATURE;
}

FORCEINLINE
BOOLEAN
LdrVerifyNtHeaders(
    _In_ PIMAGE_NT_HEADERS NtHeaders
)
{
    return NtHeaders->Signature == IMAGE_NT_SIGNATURE;
}

NTSYSAPI ULONG lstrlenW(
    _In_ PCWSTR String
);

NTSYSAPI LONG lstrcmpW(
    _In_ PCWSTR String1,
    _In_ PCWSTR String2
);

NTSYSAPI LONG lstrcmpiW(
    _In_ PCWSTR String1,
    _In_ PCWSTR String2
);

NTSYSAPI LONG lstrncmpW(
    _In_ PCWSTR String1,
    _In_ PCWSTR String2,
    _In_ ULONG  Characters
);

NTSYSAPI LONG lstrncmpiW(
    _In_ PCWSTR String1,
    _In_ PCWSTR String2,
    _In_ ULONG  Characters
);

NTSYSAPI VOID lstrcpyW(
    _Inout_ PWSTR DestinationString,
    __in    PWSTR SourceString
);

NTSYSAPI VOID lstrncpyW(
    _Inout_ PWSTR DestinationString,
    __in    PWSTR SourceString,
    __in    ULONG Characters
);

NTSYSAPI VOID lstrcatW(
    _In_ PWSTR  DestinationString,
    _In_ PCWSTR SourceString
);

NTSYSAPI VOID lstrncatW(
    _In_ PWSTR  DestinationString,
    _In_ PCWSTR SourceString,
    _In_ ULONG  Characters
);

NTSYSAPI PWSTR lstrchrW(
    _In_ PWSTR String,
    _In_ WCHAR Character
);

NTSYSAPI PWSTR lstrchriW(
    _In_ PWSTR String,
    _In_ WCHAR Character
);

NTSYSAPI PWSTR lstrstrW(
    _In_ PWSTR String,
    _In_ PWSTR Substring
);

NTSYSAPI PWSTR lstrstriW(
    _In_ PWSTR String,
    _In_ PWSTR Substring
);

NTSYSAPI
VOID
RtlAssertionFailure(
    _In_ PWSTR Assert
);

NTSYSAPI
NTSTATUS
RtlUnwindFrame(
    _In_ PKTHREAD        Thread,
    _In_ PCONTEXT        TargetContext,
    _In_ ULONG64         StackBase,
    _In_ ULONG64         StackLength
);

NTSYSAPI
NORETURN
VOID
RtlRaiseException(
    _In_ NTSTATUS Exception
);

NTSYSAPI
VOID
__C_specific_handler(
    _In_ PEXCEPTION_RECORD Record,
    _In_ PUNWIND_INFO      Unwind
);

NTSYSAPI
ULONG64
__chkstk(

);
