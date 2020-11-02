/*++

Module ObjectName:

	carbsup.h

Abstract:

	Defines public data structures and procedure prototypes for supervisor
	modules to include.

--*/

#pragma once

#pragma warning (disable : 4200)
#pragma warning (disable : 4201)
#pragma warning (disable : 4204)
#pragma warning (disable : 4152)
#pragma warning (disable : 4214)
#pragma warning (disable : 4366)
#pragma warning (disable : 4213)

#define NULL				((void*)0)
#define VOID				void

#define EXTERN				extern
#define STATIC				static
#define VOLATILE			volatile
#define DECLSPEC			__declspec
#define CONST				const
#define DLLIMPORT			dllimport
#define DLLEXPORT			dllexport
#define FORCEINLINE			__forceinline
#define UNALIGNED			__unaligned

typedef unsigned char		UCHAR, *PUCHAR;
typedef unsigned short		USHORT, *PUSHORT;
typedef unsigned long		ULONG, *PULONG;
typedef unsigned long		ULONG32, *PULONG32;
typedef unsigned long long	ULONG64, *PULONG64;

typedef signed char			CHAR, *PCHAR;
typedef signed short		SHORT, *PSHORT;
typedef signed long			LONG, *PLONG;
typedef signed long			LONG32, *PLONG32;
typedef signed long	long	LONG64, *PLONG64;

typedef unsigned long long	UCHAR_PTR;
typedef unsigned long long	USHORT_PTR;
typedef unsigned long long	ULONG_PTR;
typedef unsigned long long	ULONG32_PTR;
typedef unsigned long long	ULONG64_PTR;

typedef unsigned char		BOOLEAN, *PBOOLEAN;

#define TRUE				1
#define FALSE				0

typedef unsigned short		wchar_t;
typedef unsigned short		WCHAR, *PWCHAR, *PWSTR;
typedef CONST WCHAR			*PCWCHAR, *PCWSTR;
typedef void*				PVOID;

typedef float				FLOAT;
typedef double				DOUBLE;

typedef unsigned long long	SIZE_T, *PSIZE_T;

#define FIELD_OFFSET(type, field) ((ULONG64_PTR)&(((type*)0)->field))

#ifdef __KERNEL_INTERNAL__
#define NTSYSAPI DECLSPEC(DLLEXPORT)
#else
#define NTSYSAPI DECLSPEC(DLLIMPORT)
#endif

#include <intrin.h>

typedef ULONG32 HANDLE, *PHANDLE;

#include "ntstatus.h"
#include "rtl.h"

typedef struct _KTCB *PKTCB;
typedef struct _KTHREAD *PKTHREAD;
typedef struct _KPROCESS *PKPROCESS;
typedef struct _KPCR *PKPCR;

#include "hal.h"
#include "mm.h"
#include "ke.h"
#include "ex.h"
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