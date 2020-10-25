/*++

Module ObjectName:

	rtl.h

Abstract:

	Defines run time library data structures and procedure prototypes.

--*/

#pragma once

//intrin.h
//typedef unsigned char*		va_list;

#define stack_size			sizeof(void*)
#define va_size(type)		((sizeof(type)+stack_size-1)&~(stack_size-1))
#define va_start(ap, la)	(ap=((va_list)(&(la))+va_size(la)))
#define va_end(ap)			(ap=(va_list)0)
#define va_arg(ap, type)	(ap+=va_size(type),*((type*)(ap-va_size(type))))


typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWCHAR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef CONST UNICODE_STRING *PCUNICODE_STRING;

#define UNICODE_NULL ((WCHAR)0)

#define UNICODE_STRING_MAX_BYTES ((USHORT)65534)
#define UNICODE_STRING_MAX_CHARS ((USHORT)32767)

#define RTL_CONSTANT_UNICODE_STRING(s) {(sizeof(s)-2)/sizeof(wchar_t), sizeof(s), s}

#define UPPER(c)		((c) >= 0x61 && (c) <= 0x7A ? (c) - 0x20 : (c))

NTSYSAPI
VOID
RtlInitUnicodeString(
	__out PUNICODE_STRING DestinationString,
	__in PWSTR SourceString
);

NTSYSAPI
ULONG
RtlStringLength(
	__in PCWSTR SourceString
);

NTSYSAPI
USHORT
RtlStringCompare(
	__in PCWSTR String1,
	__in PCWSTR String2
);

NTSYSAPI
VOID
RtlStringCopy(
	__out PWSTR DestinationString,
	__in PWSTR SourceString
);

NTSYSAPI
VOID
RtlStringCopyLength(
	__out PWSTR DestinationString,
	__in PWSTR SourceString,
	__in ULONG Characters
);

NTSYSAPI
USHORT
RtlStringCompareLength(
	__in PCWSTR String1,
	__in PCWSTR String2,
	__in ULONG Characters
);

NTSYSAPI
NTSTATUS
RtlUnicodeStringValidate(
	__in PCUNICODE_STRING SourceString
);

NTSYSAPI
NTSTATUS
RtlUnicodeStringCopy(
	__out PUNICODE_STRING DestinationString,
	__in PCUNICODE_STRING SourceString
);

NTSYSAPI
ULONG
RtlUnicodeStringCompare(
	__in PUNICODE_STRING String1,
	__in PUNICODE_STRING String2
);

NTSYSAPI
ULONG
RtlUnicodeStringCompareLength(
	__in PUNICODE_STRING String1,
	__in PUNICODE_STRING String2,
	__in ULONG Characters
);

NTSYSAPI
VOID
RtlAllocateAndInitUnicodeStringEx(
	__out PUNICODE_STRING* AllocatedString,
	__in PWSTR SourceString
);

NTSYSAPI
VOID
RtlAllocateAndInitUnicodeString(
	__out PUNICODE_STRING AllocatedString,
	__in PWSTR SourceString
);

NTSYSAPI
VOID
RtlFreeUnicodeString(
	__in PUNICODE_STRING AllocatedString
);


NTSYSAPI void* _memset( void* m1, unsigned char v, __int64 n );
NTSYSAPI void* _memcpy( void* dst, void* src, int n );
NTSYSAPI unsigned char _memcmp( void* m1, void* m2, int n );
NTSYSAPI int _strcmp( char* str1, char* str2 );
NTSYSAPI int _wcscmp( wchar_t* str1, wchar_t* str2 );
NTSYSAPI int _strlen( char* str1 );
NTSYSAPI int _wcslen( wchar_t* str1 );
NTSYSAPI char* strrev( char* str1 );
NTSYSAPI wchar_t* wcsrev( wchar_t* str1 );
NTSYSAPI char* itoa( int n, char* str1, unsigned int base );
NTSYSAPI wchar_t* itow( int n, wchar_t* str1, unsigned int base );
NTSYSAPI unsigned int atoi( char* str1 );
NTSYSAPI unsigned int wtoi( wchar_t* str1 );
NTSYSAPI void vsprintfA( char* buffer, char* format, va_list args );
NTSYSAPI void vsprintfW( wchar_t* buffer, wchar_t* format, va_list args );
NTSYSAPI void sprintfA( char* buffer, char* format, ... );
NTSYSAPI void sprintfW( wchar_t* buffer, wchar_t* format, ... );
NTSYSAPI int strncmp( char* str1, char* str2, int n );
NTSYSAPI int wcsncmp( wchar_t* str1, wchar_t* str2, int n );
NTSYSAPI float _pow( float n, int ex );
NTSYSAPI char* ftoa( float n, char* str1, unsigned int precision );
NTSYSAPI wchar_t* ftow( float n, wchar_t* str1, unsigned int precision );