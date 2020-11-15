


#pragma once

#include <intrin.h>

#pragma warning (disable: 4200)
#pragma warning (disable: 4201)
#pragma warning (disable: 4204)
#pragma warning (disable: 4152)
#pragma warning (disable: 4214)
#pragma warning (disable: 4366)
#pragma warning (disable: 4213)

#define NULL				((void *)0)
#define VOID				void

#define TRUE				1
#define FALSE				0

#define CONST				const
#define VOLATILE			volatile
#define EXTERN				extern
#define STATIC				static
#define FORCEINLINE			__forceinline
#define UNALIGNED			__unaligned
#define DECLSPEC			__declspec
#define DLLIMPORT			dllimport
#define DLLEXPORT			dllexport
#define OPTIONAL

#define FIELD_OFFSET(type, field)		((ULONG64_PTR)&(((type *)0)->field))

#define EXCEPTION_EXECUTE_HANDLER		 1
#define EXCEPTION_CONTINUE_SEARCH		 0
#define EXCEPTION_CONTINUE_EXECUTION	-1

#define NTSYSAPI DECLSPEC(DLLIMPORT)

typedef unsigned char		byte;
typedef unsigned short		wchar_t;

typedef signed char			CHAR, *PCHAR;
typedef signed short		SHORT, *PSHORT;
typedef signed int			INT, *PINT;
typedef signed long			LONG, *PLONG;
typedef signed long			LONG32, *PLONG32;
typedef signed long	long	LONG64, *PLONG64;

typedef unsigned char		UCHAR, *PUCHAR;
typedef unsigned short		USHORT, *PUSHORT;
typedef unsigned int		UINT, *PUINT;
typedef unsigned long		ULONG, *PULONG;
typedef unsigned long		ULONG32, *PULONG32;
typedef unsigned long long	ULONG64, *PULONG64;

typedef unsigned char		BOOLEAN, *PBOOLEAN;
typedef unsigned long		HANDLE, *PHANDLE;
typedef unsigned long long	SIZE_T, *PSIZE_T;
typedef unsigned long long	UCHAR_PTR,
USHORT_PTR,
ULONG_PTR,
ULONG32_PTR,
ULONG64_PTR;

typedef __m128i				M128I, *PM128I;
typedef void							*PVOID;

typedef unsigned char		BYTE, *PBYTE;
typedef unsigned short		WCHAR, *PWCHAR, *PWSTR;
typedef const  wchar_t					*PCWCHAR, *PCWSTR;

typedef float				FLOAT;
typedef double				DOUBLE;

#include "ntstatus.h"

typedef struct _UNICODE_STRING {
    ULONG Length;
    ULONG Size;
    PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef CONST UNICODE_STRING *PCUNICODE_STRING;

#define RTL_CONSTANT_UNICODE_STRING(s) {(sizeof(s)-2)/sizeof(wchar_t), sizeof(s), s}

typedef ULONG32 ACCESS_MASK;
typedef ACCESS_MASK *PACCESS_MASK;

typedef struct _IO_STATUS_BLOCK {
    NTSTATUS Status;
    ULONG64 Information;

} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef enum _FILE_INFORMATION_CLASS {
    FileDirectoryInformation = 1,
    FileBasicInformation,

} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

typedef struct _FILE_BASIC_INFORMATION {
    ULONG64 FileSize;
    ULONG64 FileSizeOnDisk;

} FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;

typedef struct _OBJECT_ATTRIBUTES {
    ULONG32 Attributes;
    PUNICODE_STRING ObjectName;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;


