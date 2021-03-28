


#pragma once

//
// Native api header
//

#pragma warning(disable:4200)
#pragma warning(disable:4201)
#pragma warning(disable:4213)
#pragma warning(disable:4214)
#pragma warning(disable:4701)
#pragma warning(disable:4053)
#pragma warning(disable:4152)

// format
#pragma warning(disable:4477)
#pragma warning(disable:4313)

#undef _WIN32

#include <ntbase.h>
#include <ntstatus.h>

typedef signed char        int8_t;
typedef short              int16_t;
typedef int                int32_t;
typedef long long          int64_t;
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

// change
#include "../../core/ntuser/usersup.h"

#undef NTSYSAPI
#ifdef NTDLL_INTERNAL
#define NTSYSAPI DLLEXPORT
#else
#define NTSYSAPI DLLIMPORT
#endif

#ifdef USER_INTERNAL
#define NTUSRAPI DLLEXPORT
#else
#define NTUSRAPI DLLIMPORT
#endif

NTSYSAPI
NTSTATUS
NtCreateFile(
    _Out_ PHANDLE            FileHandle,
    _Out_ PIO_STATUS_BLOCK   IoStatusBlock,
    _In_  ACCESS_MASK        DesiredAccess,
    _In_  POBJECT_ATTRIBUTES ObjectAttributes,
    _In_  ULONG              Disposition,
    _In_  ULONG              ShareAccess,
    _In_  ULONG              CreateOptions
);

NTSYSAPI
NTSTATUS
NtReadFile(
    _In_    HANDLE           FileHandle,
    _In_    HANDLE           EventHandle,
    _Inout_ PIO_STATUS_BLOCK StatusBlock,
    _Out_   PVOID            Buffer,
    _In_    ULONG64          Length,
    _In_    ULONG64          Offset
);

NTSYSAPI
NTSTATUS
NtDeviceIoControlFile(
    _In_  HANDLE           FileHandle,
    _In_  HANDLE           EventHandle,
    _Out_ PIO_STATUS_BLOCK StatusBlock,
    _In_  ULONG32          ControlCode,
    _Out_ PVOID            InputBuffer,
    _In_  ULONG64          InputLength,
    _In_  PVOID            OutputBuffer,
    _In_  ULONG64          OutputLength
);

NTSYSAPI
NTSTATUS
NtCreateSection(
    _Out_    PHANDLE            SectionHandle,
    _In_     ACCESS_MASK        DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_     ULONG              AllocationAttributes,
    _In_opt_ HANDLE             FileHandle
);

NTSYSAPI
NTSTATUS
NtMapViewOfSection(
    _In_  HANDLE  SectionHandle,
    _In_  HANDLE  ProcessHandle,
    _Out_ PVOID*  BaseAddress,
    _In_  ULONG64 ViewOffset,
    _In_  ULONG64 ViewLength,
    _In_  ULONG   Protection
);

NTSYSAPI
NTSTATUS
NtUnmapViewOfSection(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID  BaseAddress
);

NTSYSAPI
NTSTATUS
NtResizeSection(
    _In_ HANDLE  SectionHandle,
    _In_ ULONG64 SectionLength
);

NTSYSAPI
NTSTATUS
NtClose(
    _In_ HANDLE Handle
);

NTSYSAPI
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

NTSYSAPI
ULONG64
__chkstk(

);

typedef struct _SETJMP_FLOAT128
{
    unsigned __int64 Part[ 2 ];
} SETJMP_FLOAT128;

#define _JBLEN  16
typedef SETJMP_FLOAT128 _JBTYPE;

typedef struct _JUMP_BUFFER
{
    unsigned __int64 Frame;
    unsigned __int64 Rbx;
    unsigned __int64 Rsp;
    unsigned __int64 Rbp;
    unsigned __int64 Rsi;
    unsigned __int64 Rdi;
    unsigned __int64 R12;
    unsigned __int64 R13;
    unsigned __int64 R14;
    unsigned __int64 R15;
    unsigned __int64 Rip;
    unsigned long MxCsr;
    unsigned short FpCsr;
    unsigned short Spare;

    SETJMP_FLOAT128 Xmm6;
    SETJMP_FLOAT128 Xmm7;
    SETJMP_FLOAT128 Xmm8;
    SETJMP_FLOAT128 Xmm9;
    SETJMP_FLOAT128 Xmm10;
    SETJMP_FLOAT128 Xmm11;
    SETJMP_FLOAT128 Xmm12;
    SETJMP_FLOAT128 Xmm13;
    SETJMP_FLOAT128 Xmm14;
    SETJMP_FLOAT128 Xmm15;
} _JUMP_BUFFER;

#ifndef _JMP_BUF_DEFINED
#define _JMP_BUF_DEFINED
typedef _JBTYPE jmp_buf[ _JBLEN ];
#endif

NTSYSAPI int __cdecl setjmp(
    _Out_ jmp_buf _Buf
);

NTSYSAPI NORETURN void __cdecl longjmp(
    _In_ jmp_buf _Buf,
    _In_ int     _Value
);

#ifndef NTDLL_INTERNAL
//
// intrins
//

NTSYSAPI int        strcmp( const char* str1, const char* str2 );
NTSYSAPI size_t     strlen( const char* str1 );
NTSYSAPI char*      strcpy( char* destination, const char* source );
NTSYSAPI char*      strcat( char* destination, const char* source );

NTSYSAPI void*      memchr( void* ptr, int value, size_t num );
NTSYSAPI int        memcmp( const void* ptr1, const void* ptr2, size_t num );
NTSYSAPI void*      memcpy( void* destination, const void* source, size_t num );
NTSYSAPI void*      memset( void* mem, int v, size_t num );
NTSYSAPI void*      memmove( void* destination, const void* source, size_t num );

NTSYSAPI size_t     wcslen( const wchar_t* string );
NTSYSAPI wchar_t*   wcscpy( wchar_t* destination, const wchar_t* source );

#else

int        strcmp( const char* str1, const char* str2 );
size_t     strlen( const char* str1 );
char*      strcpy( char* destination, const char* source );
char*      strcat( char* destination, const char* source );

void*      memchr( void* ptr, int value, size_t num );
int        memcmp( const void* ptr1, void* ptr2, size_t num );
void*      memcpy( void* destination, void* source, size_t num );
void*      memset( void* mem, int v, size_t num );
void*      memmove( void* destination, void* source, size_t num );

size_t     wcslen( const wchar_t* string );
wchar_t*   wcscpy( wchar_t* destination, const wchar_t* source );

#endif

NTSYSAPI int        strncmp( const char* str1, const char* str2, size_t num );
NTSYSAPI char*      strdup( char* string );
NTSYSAPI const char*strrchr( const char* str, int character );
NTSYSAPI const char*strstr( const char *s1, const char *s2 );
NTSYSAPI char*      strncpy( char* destination, const char* source, size_t num );
NTSYSAPI long int   strtol( const char* str, char** end, int base );

NTSYSAPI wchar_t*   wcscat( wchar_t* destination, const wchar_t* source );
NTSYSAPI wchar_t*   wcsdup( wchar_t* string );
NTSYSAPI wchar_t*   wcsrev( wchar_t* String );

NTSYSAPI size_t     mbstowcs( wchar_t* wcstr, const char* mbstr, size_t count );

NTSYSAPI void       itow( int Value, wchar_t* Buffer, int Base );
NTSYSAPI int        wtoi( const wchar_t* Buffer );
NTSYSAPI int        vswprintf( wchar_t* Buffer, const wchar_t* Format, va_list List );
NTSYSAPI int        swprintf( wchar_t* buffer, const wchar_t* format, ... );
NTSYSAPI int        sprintf( char* buffer, const char* format, ... );
NTSYSAPI int        vsprintf( char* Buffer, const char* Format, va_list List );

NTSYSAPI int        atoi( const char* Buffer );

NTSYSAPI void       qsort( void* base, size_t num, size_t size, int( *compare )( const void*, const void* ) );

#define EOF ( -1 )

#ifndef _FILE_DEFINED
#define _FILE_DEFINED
typedef struct _FILE {

    HANDLE FileHandle;
    ACCESS_MASK Access;
    BOOLEAN AppendMode;
    long int Offset; // "long int"

} FILE, *PFILE;
#endif

#define stdin  ( NtCurrentPeb( )->FileStdin )
#define stdout ( NtCurrentPeb( )->FileStdout )
#define stderr ( NtCurrentPeb( )->FileStderr )

NTSYSAPI FILE*      fopen( const char* filename, const char* mode );
NTSYSAPI int        fclose( FILE* stream );
NTSYSAPI size_t     fread( void* ptr, size_t size, size_t count, FILE* stream );
NTSYSAPI int        fseek( FILE* stream, long int offset, int origin );
NTSYSAPI long int   ftell( FILE* stream );
NTSYSAPI int        vfprintf( FILE* stream, const char* format, va_list arg );
NTSYSAPI int        printf( const char* format, ... );

#define SEEK_SET ( 0 )
#define SEEK_CUR ( 1 )
#define SEEK_END ( 2 )

#define toupper( c )               ( ( c ) >= 'a' && ( c ) <= 'z' ? ( c ) - ' ' : ( c ) )
#define tolower( c )               ( ( c ) >= 'A' && ( c ) <= 'Z' ? ( c ) + ' ' : ( c ) )

NTSYSAPI void*      malloc( size_t size );
NTSYSAPI void       free( void* ptr );
NTSYSAPI void*      calloc( size_t num, size_t size );
NTSYSAPI void*      realloc( void* ptr, size_t new_size );

NTSYSAPI char*      getenv( const char* name );

NTSYSAPI
NTSTATUS
LdrLoadDll(
    _In_  PWSTR  DirectoryFile,
    _Out_ PVOID* ModuleHandle
);

NTSYSAPI
VOID
RtlDebugPrint(
    _In_ PWSTR Format,
    _In_ ...
);

NTSYSAPI
NTSTATUS
NtWaitForSingleObject(
    _In_ HANDLE  ObjectHandle,
    _In_ ULONG64 TimeOut
);

NTSYSAPI
NTSTATUS
NtAllocateVirtualMemory(
    _In_    HANDLE  ProcessHandle,
    _Inout_ PVOID*  BaseAddress,
    _In_    ULONG64 Length,
    _In_    ULONG32 Protect
);

NTSYSAPI
NTSTATUS
NtFreeVirtualMemory(
    _In_ HANDLE  ProcessHandle,
    _In_ PVOID   BaseAddress,
    _In_ ULONG64 Length
);

NTSYSAPI
NTSTATUS
NtQuerySystemClock(
    _In_ PKSYSTEM_TIME ClockTime
);

NTSYSAPI
NTSTATUS
NtCreateMutex(
    _Out_ PHANDLE MutexHandle,
    _In_  BOOLEAN InitialOwner
);

NTSYSAPI
NTSTATUS
NtReleaseMutex(
    _In_ HANDLE MutexHandle
);

NTSYSAPI
PVOID
RtlCreateHeap(

);

NTSYSAPI
PVOID
RtlAllocateHeap(
    _In_ PVOID   Heap,
    _In_ ULONG64 Length
);

NTSYSAPI
VOID
RtlFreeHeap(
    _In_ PVOID Heap,
    _In_ PVOID Memory
);

NTSYSAPI
ULONG64
RtlAllocationSizeHeap(
    _In_ PVOID Heap,
    _In_ PVOID Memory
);

NTSYSAPI
VOID
LdrInitializeProcess(

);

NTSYSAPI
NTSTATUS
NtQueryInformationFile(
    _In_  HANDLE                 FileHandle,
    _Out_ PIO_STATUS_BLOCK       StatusBlock,
    _Out_ PVOID                  FileInformation,
    _In_  ULONG64                Length,
    _In_  FILE_INFORMATION_CLASS FileInformationClass
);

NTSYSAPI
NTSTATUS
NtDirectorySplit(
    _In_  PWSTR InputBuffer,
    _Out_ PWSTR ObjectName,
    _Out_ PWSTR RootDirectory
);

typedef struct _NT_FONT_HANDLE *PNT_FONT_HANDLE;

NTUSRAPI
NTSTATUS
NtCreateFont(
    _Out_ PNT_FONT_HANDLE* FontHandle,
    _In_  ULONG32          Height,
    _In_  ULONG32          Width,
    _In_  PWSTR            FaceName
);

NTUSRAPI
NTSTATUS
NtDrawText(
    _In_ HANDLE          ContextHandle,
    _In_ PNT_FONT_HANDLE FontHandle,
    _In_ PWSTR           DrawText,
    _In_ PRECT           Rect,
    _In_ ULONG32         Flags,
    _In_ ULONG32         Colour
);

NTUSRAPI
VOID
NtInitializeUser(

);
