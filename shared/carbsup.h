/*++

Module ObjectName:

	carbsup.h

Abstract:

	Defines public data structures and procedure prototypes for supervisor
	modules to include.

--*/

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

#ifdef __KERNEL_INTERNAL__
	#define NTSYSAPI DECLSPEC(DLLEXPORT)
#else
	#define NTSYSAPI DECLSPEC(DLLIMPORT)
#endif

typedef unsigned char		byte;
typedef unsigned short		wchar_t;

typedef signed char			CHAR,		*PCHAR;
typedef signed short		SHORT,		*PSHORT;
typedef signed int			INT,		*PINT;
typedef signed long			LONG,		*PLONG;
typedef signed long			LONG32,		*PLONG32;
typedef signed long	long	LONG64,		*PLONG64;

typedef unsigned char		UCHAR,		*PUCHAR;
typedef unsigned short		USHORT,		*PUSHORT;
typedef unsigned int		UINT,		*PUINT;
typedef unsigned long		ULONG,		*PULONG;
typedef unsigned long		ULONG32,	*PULONG32;
typedef unsigned long long	ULONG64,	*PULONG64;

typedef unsigned char		BOOLEAN,	*PBOOLEAN;
typedef unsigned long		HANDLE,		*PHANDLE;
typedef unsigned long long	SIZE_T,		*PSIZE_T;
typedef unsigned long long	UCHAR_PTR,
							USHORT_PTR, 
							ULONG_PTR,
							ULONG32_PTR,
							ULONG64_PTR;

typedef __m128i				M128I,		*PM128I;
typedef void							*PVOID;

typedef unsigned char		BYTE,		*PBYTE;
typedef unsigned short		WCHAR,		*PWCHAR,  *PWSTR;
typedef const  wchar_t					*PCWCHAR, *PCWSTR;

typedef float				FLOAT;
typedef double				DOUBLE;

typedef struct				_KTCB		*PKTCB;
typedef struct				_KTHREAD	*PKTHREAD;
typedef struct				_KPROCESS	*PKPROCESS;
typedef struct				_KPCR		*PKPCR;
typedef struct				_CONTEXT	*PCONTEXT;

#include "ntstatus.h"
#include "rtl.h"

#include "hal.h"
#include "mm.h"
#include "ex.h"
#include "ke.h"
#include "ldrsup.h"
#include "ob.h"
#include "io.h"
#include "dbg.h"
#include "ps.h"
#include "zw.h"

NTSYSAPI
VOID
printf(
	__in CHAR* Format,
	__in ...
);

NTSYSAPI
VOID
HalpDrawString(
	__in CHAR* String,
	__in ULONG x,
	__in ULONG y,
	__in ULONG Foreground
);