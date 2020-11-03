/*++

Module ObjectName:

	rtl.h

Abstract:

	Defines run time library data structures and procedure prototypes.

--*/

#pragma once

#define stack_size			sizeof(void *)
#define va_size(type)		((sizeof(type) + stack_size - 1) & ~(stack_size - 1))
#define va_start(ap, la)	(ap = ((va_list)(&(la)) + va_size(la)))
#define va_end(ap)			(ap = (va_list)0)
#define va_arg(ap, type)	(ap += va_size(type), *((type *)(ap - va_size(type))))

#define tolower(c)			((c) >= 'A' && (c) <= 'Z' ? (c) + ' ' : (c))
#define toupper(c)			((c) >= 'a' && (c) <= 'z' ? (c) - ' ' : (c))

#define RTL_CONSTANT_UNICODE_STRING(s) {(sizeof(s)-2)/sizeof(wchar_t), sizeof(s), s}

typedef struct _UNICODE_STRING {
	ULONG Length;
	ULONG Size;
	PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef CONST UNICODE_STRING *PCUNICODE_STRING;

NTSYSAPI ULONG lstrlenW(
	__in PCWSTR String
);

NTSYSAPI LONG lstrcmpW(
	__in PCWSTR String1,
	__in PCWSTR String2
);

NTSYSAPI LONG lstrcmpiW(
	__in PCWSTR String1,
	__in PCWSTR String2
);

NTSYSAPI LONG lstrncmpW(
	__in PCWSTR String1,
	__in PCWSTR String2,
	__in ULONG	Characters
);

NTSYSAPI LONG lstrncmpiW(
	__in PCWSTR String1,
	__in PCWSTR String2,
	__in ULONG	Characters
);

NTSYSAPI VOID lstrcpyW(
	__inout PWSTR DestinationString,
	__in	PWSTR SourceString
);

NTSYSAPI VOID lstrncpyW(
	__inout PWSTR DestinationString,
	__in	PWSTR SourceString,
	__in	ULONG Characters
);

NTSYSAPI VOID lstrcatW(
	__in PWSTR	DestinationString,
	__in PCWSTR SourceString
);

NTSYSAPI VOID lstrncatW(
	__in PWSTR	DestinationString,
	__in PCWSTR SourceString,
	__in ULONG	Characters
);

NTSYSAPI PWSTR lstrchrW(
	__in PWSTR String,
	__in WCHAR Character
);

NTSYSAPI PWSTR lstrchriW(
	__in PWSTR String,
	__in WCHAR Character
);

NTSYSAPI PWSTR lstrstrW(
	__in PWSTR String,
	__in PWSTR Substring
);

NTSYSAPI PWSTR lstrstriW(
	__in PWSTR String,
	__in PWSTR Substring
);

PWSTR lstrbrkW(
	__in PWSTR String,
	__in WCHAR *Delimiters
);

PWSTR lstrtokW(
	__in PWSTR String,
	__in WCHAR *Delimiters
);

NTSYSAPI NTSTATUS RtlUnicodeStringValidate(
	__in PUNICODE_STRING UnicodeString
);

NTSYSAPI VOID RtlInitUnicodeString(
	__inout PUNICODE_STRING	StringInformation,
	__in	PWSTR			String
);

NTSYSAPI VOID RtlAllocateAndInitUnicodeString(
	__inout PUNICODE_STRING	AllocatedString,
	__in	PWSTR			SourceString
);

NTSYSAPI VOID RtlAllocateAndInitUnicodeStringEx(
	__inout PUNICODE_STRING	*AllocatedString,
	__in	PWSTR			SourceString
);

NTSYSAPI VOID RtlFreeUnicodeString(
	__in PUNICODE_STRING AllocatedString
);

NTSYSAPI NTSTATUS RtlUnicodeStringCopy(
	__inout PUNICODE_STRING	DestinationUnicodeString,
	__in	PUNICODE_STRING	SourceUnicodeString
);

NTSYSAPI NTSTATUS RtlUnicodeStringCopyLength(
	__inout PUNICODE_STRING	DestinationUnicodeString,
	__in	PUNICODE_STRING	SourceUnicodeString,
	__in	ULONG			Characters
);

NTSYSAPI ULONG RtlUnicodeStringCompare(
	__in PUNICODE_STRING UnicodeString1,
	__in PUNICODE_STRING UnicodeString2
);

NTSYSAPI ULONG RtlUnicodeStringCompareLength(
	__in PUNICODE_STRING	UnicodeString1,
	__in PUNICODE_STRING	UnicodeString2,
	__in ULONG				Characters
);

/* CRT functions. */
NTSYSAPI void *_memset( void *m1, unsigned char v, __int64 n );
NTSYSAPI void *_memcpy( void *dst, void *src, int n );
NTSYSAPI unsigned char _memcmp( void *m1, void *m2, int n );
NTSYSAPI int _strcmp( char *str1, char *str2 );
NTSYSAPI int _wcscmp( wchar_t *str1, wchar_t *str2 );
NTSYSAPI int _strlen( char *str1 );
NTSYSAPI int _wcslen( wchar_t *str1 );
NTSYSAPI char *strrev( char *str1 );
NTSYSAPI wchar_t *wcsrev( wchar_t *str1 );
NTSYSAPI char *itoa( int n, char *str1, unsigned int base );
NTSYSAPI wchar_t *itow( int n, wchar_t *str1, unsigned int base );
NTSYSAPI unsigned int atoi( char *str1 );
NTSYSAPI unsigned int wtoi( wchar_t *str1 );
NTSYSAPI void vsprintfA( char *buffer, char *format, va_list args );
NTSYSAPI void vsprintfW( wchar_t *buffer, wchar_t *format, va_list args );
NTSYSAPI void sprintfA( char *buffer, char *format, ... );
NTSYSAPI void sprintfW( wchar_t *buffer, wchar_t *format, ... );
NTSYSAPI int strncmp( char *str1, char *str2, int n );
NTSYSAPI int wcsncmp( wchar_t *str1, wchar_t *str2, int n );
NTSYSAPI float _pow( float n, int ex );
NTSYSAPI char *ftoa( float n, char *str1, unsigned int precision );
NTSYSAPI wchar_t *ftow( float n, wchar_t *str1, unsigned int precision );

//
//	non crt shit
//

NTSYSAPI
NTSTATUS
RtlUnwind(
	__in PKTHREAD Thread,
	__in PCONTEXT TargetContext
);
