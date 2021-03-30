


#pragma once

#ifndef NULL
#define NULL 0
#endif

#define _CRT_FUNCTIONS_REQUIRED 0

//#include <vadefs.h>
//#include "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.16.27023\include\vadefs.h"
//#include <sal.h>
//#include "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.16.27023\include\sal.h"
#include "compiler.h"

#define INT8_MIN         (-127i8 - 1)
#define INT16_MIN        (-32767i16 - 1)
#define INT32_MIN        (-2147483647i32 - 1)
#define INT64_MIN        (-9223372036854775807i64 - 1)
#define INT8_MAX         127i8
#define INT16_MAX        32767i16
#define INT32_MAX        2147483647i32
#define INT64_MAX        9223372036854775807i64
#define UINT8_MAX        0xffui8
#define UINT16_MAX       0xffffui16
#define UINT32_MAX       0xffffffffui32
#define UINT64_MAX       0xffffffffffffffffui64

#define MB_LEN_MAX    5             // max. # bytes in multibyte char
#define SHRT_MIN    (-32768)        // minimum (signed) short value
#define SHRT_MAX      32767         // maximum (signed) short value
#define USHRT_MAX     0xffff        // maximum unsigned short value
#define INT_MIN     (-2147483647 - 1) // minimum (signed) int value
#define INT_MAX       2147483647    // maximum (signed) int value
#define UINT_MAX      0xffffffff    // maximum unsigned int value
#define LONG_MIN    (-2147483647L - 1) // minimum (signed) long value
#define LONG_MAX      2147483647L   // maximum (signed) long value
#define ULONG_MAX     0xffffffffUL  // maximum unsigned long value
#define LLONG_MAX     9223372036854775807i64       // maximum signed long long int value
#define LLONG_MIN   (-9223372036854775807i64 - 1)  // minimum signed long long int value
#define ULLONG_MAX    0xffffffffffffffffui64       // maximum unsigned long long int value

typedef unsigned long long size_t;

unsigned __int64 __readmsr( unsigned long );
void __writemsr( unsigned long, unsigned __int64 );
unsigned __int64 __readeflags( void );
void _disable( void );
void _enable( void );
unsigned __int64 __readcr0( void );
unsigned __int64 __readcr2( void );
unsigned __int64 __readcr3( void );
unsigned __int64 __readcr4( void );
unsigned __int64 __readcr8( void );
void __writecr0( unsigned __int64 );
void __writecr3( unsigned __int64 );
void __writecr4( unsigned __int64 );
void __writecr8( unsigned __int64 );
void __halt( void );
void __lidt( void * Source );
void _lgdt( void* );
__int64 _InterlockedCompareExchange64( __int64 volatile * _Destination, __int64 _Exchange, __int64 _Comparand );
__int64 _InterlockedIncrement64( __int64 volatile * );

void __cpuid(
    int cpuInfo[ 4 ],
    int function_id
);

void __cpuidex(
    int cpuInfo[ 4 ],
    int function_id,
    int subfunction_id
);

void __invlpg(
    void* Address
);

void _invpcid(
    unsigned __int32 type,
    void* descriptor
);

#ifdef __cplusplus
extern "C" {
#endif

    void* __cdecl _alloca( _In_ size_t _Size );
    void __stosb( unsigned char *, unsigned char, size_t );

#ifdef __cplusplus
}
#endif


#ifndef va_arg
#define va_arg __crt_va_arg
#endif

#ifndef va_start
#define va_start __crt_va_start
#endif

#ifndef va_end
#define va_end __crt_va_end
#endif

#ifndef __cplusplus
typedef unsigned short          wchar_t;
#endif

#ifndef __cplusplus
extern void*                    memcpy( void*, void*, size_t );
extern void*                    memset( void*, int, size_t );

extern size_t                   wcslen( const wchar_t* );
extern int                      wcscmp( const wchar_t*, const wchar_t* );

extern size_t                   strlen( const char* );
extern int                      strcmp( const char*, const char* );
#else
extern "C" void*                memcpy( void*, void*, size_t );
extern "C" void*                memset( void*, int, size_t );

extern "C" size_t               wcslen( const wchar_t* );
extern "C" int                  wcscmp( const wchar_t*, const wchar_t* );

extern "C" size_t               strlen( const char* );
extern "C" int                  strcmp( const char*, const char* );
#endif

typedef unsigned long long      NTSTATUS, *PNTSTATUS;

typedef signed char             CHAR, *PCHAR, *PSTR;
typedef signed short            SHORT, *PSHORT;
typedef signed int              INT, *PINT;
typedef signed long             LONG, *PLONG;
typedef signed long             LONG32, *PLONG32;
typedef signed long long        LONG64, *PLONG64;

typedef unsigned char           UCHAR, *PUCHAR;
typedef unsigned short          USHORT, *PUSHORT;
typedef unsigned int            UINT, *PUINT;
typedef unsigned long           ULONG, *PULONG;
typedef unsigned long           ULONG32, *PULONG32;
typedef unsigned long long      ULONG64, *PULONG64;

typedef unsigned char           BOOLEAN, *PBOOLEAN;
typedef long long               HANDLE, *PHANDLE;
typedef unsigned long long      SIZE_T, *PSIZE_T;
typedef unsigned long long      UCHAR_PTR, USHORT_PTR, ULONG_PTR, ULONG32_PTR, ULONG64_PTR;

typedef void                    *PVOID;
typedef const void              *PCVOID;

typedef unsigned char           BYTE, *PBYTE;
typedef unsigned short          WCHAR, *PWCHAR, *PWSTR;
typedef const wchar_t           *PCWCHAR, *PCWSTR;
typedef const char              *PCCHAR, *PCSTR;

typedef float                   FLOAT;
typedef double                  DOUBLE;
typedef char*                   VA_LIST;

typedef unsigned __int64 size_t;
typedef __int64          ptrdiff_t;
typedef __int64          intptr_t;

#define CONST                   const
#define CONSTANT                const

#define EXTERN                  extern
#define STATIC                  static
#define VOLATILE                volatile
#define FORCEINLINE             __forceinline
#define UNALIGNED               __unaligned
#define DLLIMPORT               __declspec(dllimport)
#define DLLEXPORT               __declspec(dllexport)
#define NORETURN                __declspec(noreturn)
#define OPTIONAL

#define _MACRO_STRING(x)        L#x
#define MACRO_STRING(x)         _MACRO_STRING(x)

#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))

#define VOID                    void
#define TRUE                    1
#define FALSE                   0

#define FIELD_OFFSET(x, y)      ((ULONG64)&(((x *)0)->y))
#define offsetof FIELD_OFFSET
#define CONTAINING_RECORD(address, type, field) ((type*)((PCHAR)(address)-(ULONG64)(&((type *)0)->field)))

#define EXCEPTION_EXECUTE_HANDLER        1
#define EXCEPTION_CONTINUE_SEARCH        0
#define EXCEPTION_CONTINUE_EXECUTION    -1

#ifndef NTSYSAPI
#ifdef KRNLINTERNAL
#define NTSYSAPI DLLEXPORT
#else
#define NTSYSAPI DLLIMPORT
#endif
#endif

#define C_ASSERT( expression )  static_assert( expression, #expression );

typedef struct _LIST_ENTRY {
    struct _LIST_ENTRY *Flink;
    struct _LIST_ENTRY *Blink;
} LIST_ENTRY, *PLIST_ENTRY;

FORCEINLINE
VOID
KeInsertHeadList(
    _In_ PLIST_ENTRY ListHead,
    _In_ PLIST_ENTRY Entry
)
{
    PLIST_ENTRY Flink;

    Flink = ListHead->Flink;
    Entry->Flink = Flink;
    Entry->Blink = ListHead;
    Flink->Blink = Entry;
    ListHead->Flink = Entry;
}

FORCEINLINE
VOID
KeInsertTailList(
    _In_ PLIST_ENTRY ListHead,
    _In_ PLIST_ENTRY Entry
)
{
    PLIST_ENTRY Blink;

    Blink = ListHead->Blink;
    Entry->Flink = ListHead;
    Entry->Blink = Blink;
    Blink->Flink = Entry;
    ListHead->Blink = Entry;
}

FORCEINLINE
VOID
KeRemoveList(
    _In_ PLIST_ENTRY Entry
)
{
    PLIST_ENTRY Flink;
    PLIST_ENTRY Blink;

    Flink = Entry->Flink;
    Blink = Entry->Blink;

    Flink->Blink = Blink;
    Blink->Flink = Flink;
}

FORCEINLINE
VOID
KeInitializeHeadList(
    _In_ PLIST_ENTRY Entry
)
{

    Entry->Flink = Entry;
    Entry->Blink = Entry;
}

FORCEINLINE
BOOLEAN
KeEmptyList(
    _In_ PLIST_ENTRY Entry
)
{

    return ( BOOLEAN )( Entry == Entry->Flink );
}

//
// Prototype structures.
//

#define ROUND_TO_PAGES( Length )    ( ( ( ULONG64 )( Length ) + 0xFFF ) & ~0xFFF )
#define ROUND( x, y )               ( ( ( x ) + ( y ) - 1 ) & ~( ( y ) - 1 ) )

#define WAIT_TIMEOUT_INFINITE ( ( ULONG64 )-1 )

#define DPC_OBJECT_EVENT    0x00000001
#define DPC_OBJECT_THREAD   0x00000002
#define DPC_OBJECT_MUTEX    0x00000003

typedef struct _KDPC_HEADER {
    ULONG64 Type;
} KDPC_HEADER, *PKDPC_HEADER;

typedef struct _KPROCESS    *PKPROCESS;
typedef struct _KTHREAD     *PKTHREAD;
typedef NTSTATUS( *PKSTART_ROUTINE )(
    PVOID
    );
typedef struct _KDPC        *PKDPC;
typedef VOID( *PKDEFERRED_ROUTINE )(
    _In_ PKDPC Dpc,
    _In_ PVOID DeferredContext
    );

#define PASSIVE_LEVEL   0
#define DISPATCH_LEVEL  2
#define HIGH_LEVEL      3
#define IPI_LEVEL       15

typedef ULONG64 KIRQL, *PKIRQL;
typedef struct _KPCB *PKPCB;

typedef enum _KPROCESSOR_FEATURE {
    KPF_NX_ENABLED = 0,
    KPF_PCID_ENABLED,
    KPF_XSAVE_ENABLED,
    KPF_PAGE1GB_ENABLED,
    KPF_PKU_ENABLED,
    KPF_PGE_ENABLED,
    KPF_SMEP_ENABLED,
    KPF_SMAP_ENABLED,
    KPF_PAT_ENABLED,
    KPF_FXSR_ENABLED,
    KPF_SC_ENABLED,
    KPF_SSE3_ENABLED,
    KPF_MAXIMUM
} KPROCESSOR_FEATURE, *PKPROCESSOR_FEATURE;

typedef UCHAR KPROCESSOR_MODE, *PKPROCESSOR_MODE;
typedef enum _MODE {
    KernelMode,
    UserMode,
    MaximumMode
} MODE;

typedef UCHAR KPRIORITY, *PKPRIORITY;
typedef enum _PRIORITY {
    LowPriority,
    HighPriority,
    MaximumPriority
} PRIORITY;

typedef struct _KDPC {
    ULONG64            ProcessorNumber;
    PKDEFERRED_ROUTINE DeferredRoutine;
    PVOID              DeferredContext;
    ULONG64            DirectoryTableBase;
    KIRQL              DeferredIrql;
    PKDPC              DpcLink;
    KPRIORITY          Priority;
    BOOLEAN            Completed;
} KDPC, *PKDPC;

typedef struct _KEVENT {
    KDPC_HEADER Header;
    BOOLEAN     Signaled;
} KEVENT, *PKEVENT;

typedef struct _KMUTEX {
    KDPC_HEADER Header;
    PKTHREAD    Owner;

} KMUTEX, *PKMUTEX;

typedef struct _KTRAP_FRAME *PKTRAP_FRAME;

typedef struct _OBJECT_TYPE *POBJECT_TYPE;
typedef struct _OBJECT_ATTRIBUTES *POBJECT_ATTRIBUTES;
typedef struct _OBJECT_DIRECTORY *POBJECT_DIRECTORY;
typedef struct _OBJECT_HEADER *POBJECT_HEADER;
typedef struct _DEVICE_OBJECT *PDEVICE_OBJECT;
typedef struct _DRIVER_OBJECT *PDRIVER_OBJECT;

typedef struct _CONTEXT {
    ULONG64 Rax;
    ULONG64 Rcx;
    ULONG64 Rdx;
    ULONG64 Rbx;
    ULONG64 Rsp;
    ULONG64 Rbp;
    ULONG64 Rsi;
    ULONG64 Rdi;
    ULONG64 R8;
    ULONG64 R9;
    ULONG64 R10;
    ULONG64 R11;
    ULONG64 R12;
    ULONG64 R13;
    ULONG64 R14;
    ULONG64 R15;
    ULONG64 Rip;

    ULONG64 EFlags;

    ULONG64 SegCs;
    ULONG64 SegDs;
    ULONG64 SegSs;

    ULONG64 SegEs;
    ULONG64 SegFs;
    ULONG64 SegGs;

    ULONG64 Dr0;
    ULONG64 Dr1;
    ULONG64 Dr2;
    ULONG64 Dr3;
    ULONG64 Dr6;
    ULONG64 Dr7;
} CONTEXT, *PCONTEXT;

typedef UCHAR KEXCEPTION_SEVERITY, *PKEXCEPTION_SEVERITY;
typedef enum _SEVERITY {
    ExceptionFatal,
    ExceptionNormal,
    ExceptionIgnore,
    ExceptionMaximum
} SEVERITY;

typedef ULONG ACCESS_MASK, *PACCESS_MASK;

#define PROCESS_TERMINATE           (0x0001)
#define PROCESS_QUERY_INFORMATION   (0x0002)
#define PROCESS_SET_INFORMATION     (0x0004)
#define PROCESS_VM_READ             (0x0010)
#define PROCESS_VM_WRITE            (0x0020)
#define PROCESS_CREATE_THREAD       (0x0040)
#define PROCESS_VM_OPERATION        (0x0080) // alloc & protect, etc.
#define PROCESS_ALL_ACCESS          (0xFFFF)

#define THREAD_TERMINATE            (0x0001)
#define THREAD_SUSPEND_RESUME       (0x0002)
#define THREAD_GET_CONTEXT          (0x0004)
#define THREAD_SET_CONTEXT          (0x0008)
#define THREAD_ALL_ACCESS           (0xFFFF)

#define SECTION_EXTEND_SIZE         (0x0001)
#define SECTION_MAP_EXECUTE         (0x0002)
#define SECTION_MAP_READ            (0x0004)
#define SECTION_MAP_WRITE           (0x0008)
#define SECTION_QUERY               (0x0010)
#define SECTION_ALL_ACCESS          (0xFFFF)

#define DIRECTORY_QUERY             (0x0001)
#define DIRECTORY_TRAVERSE          (0x0002)
#define DIRECTORY_CREATE_OBJECT     (0x0004)
#define DIRECTORY_ALL_ACCESS        (0xFFFF)

#define GENERIC_ALL                 (0xE0000000)
#define SYNCHRONIZE                 (0x10000000)
#define GENERIC_READ                (0x20000000)
#define GENERIC_WRITE               (0x40000000)
#define GENERIC_EXECUTE             (0x80000000)

#define FULL_ACCESS                 (0xFFFFFFFF)

#define PAGE_READ                   (0x01000000)
#define PAGE_WRITE                  (0x02000000)
#define PAGE_EXECUTE                (0x04000000)
#define PAGE_WRITECOMBINE           (0x08000000)
#define PAGE_NOCACHE                (0x00800000)
#define PAGE_READWRITE_EXECUTE      (0x07000000)

#define SEC_READ                    (0x0000)
#define SEC_EXECUTE                 (0x0001)
#define SEC_WRITE                   (0x0002)
#define SEC_NOCACHE                 (0x0004)
#define SEC_WRITECOMBINE            (0x0008)
#define SEC_IMAGE                   (0x0010)
#define SEC_IMAGE_NO_EXECUTE        (0x0020)
#define SEC_SPECIFY_ADDRESS         (0x0040)
#define SEC_NO_EXTEND               (0x0080)

typedef struct _MM_SECTION_OBJECT *PMM_SECTION_OBJECT;
typedef struct _IO_FILE_OBJECT *PIO_FILE_OBJECT;

typedef struct _MM_VAD *PMM_VAD;

NTSYSAPI ULONG64 KeRootSerial;

#if 0
typedef enum _SYSTEM_INFORMATION_CLASS {
    SystemObjectTypeInformation
} SYSTEM_INFORMATION_CLASS;
#endif

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;

} UNICODE_STRING, *PUNICODE_STRING;
typedef CONST UNICODE_STRING *PCUNICODE_STRING;

#define RTL_CONSTANT_STRING( _ )        { sizeof( _ ) - sizeof( *_ ), sizeof( _ ) , _ }
#define RTL_STACK_STRING( String, Length )      \
String.MaximumLength = Length * sizeof( WCHAR ); \
String.Buffer = _alloca( Length * sizeof( WCHAR ) );

#define ARGUMENT_PRESENT( Argument )    ( ( ( void* )( Argument ) ) != NULL )

typedef struct _OBJECT_ATTRIBUTES {
    UNICODE_STRING ObjectName;
    UNICODE_STRING RootDirectory;
    ULONG          Attributes;

} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

typedef struct _OBJECT_DIRECTORY_INFORMATION {
    UNICODE_STRING Name;
    UNICODE_STRING TypeName;
} OBJECT_DIRECTORY_INFORMATION, *POBJECT_DIRECTORY_INFORMATION;

typedef struct _DISK_OBJECT_HEADER {
    //
    // Any disks should have this as a header
    // for their device object->device extension
    //

    ULONG64 PartCount;

} DISK_OBJECT_HEADER, *PDISK_OBJECT_HEADER;

#define ZwCurrentProcess( )     ( ( HANDLE )( -1 ) )
#define ZwCurrentThread( )      ( ( HANDLE )( -2 ) )
#define NtCurrentProcess( )     ( ( HANDLE )( -1 ) )
#define NtCurrentThread( )      ( ( HANDLE )( -2 ) )

//
// This structure is loosely defined and
// will probably change.
//

typedef struct _EXCEPTION_RECORD {
    PKTHREAD            Thread;
    PMM_VAD             Vad;
    KPROCESSOR_MODE     ProcessorMode;
    KEXCEPTION_SEVERITY ExceptionSeverity;
    CONTEXT             ExceptionContext;

    NTSTATUS            Status;
    ULONG64             Code1;
    ULONG64             Code2;
    ULONG64             Code3;
    ULONG64             Code4;

} EXCEPTION_RECORD, *PEXCEPTION_RECORD;

typedef struct _IO_STATUS_BLOCK {
    NTSTATUS Status;
    ULONG64  Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

#define FILE_SUPERSED                   1
#define FILE_CREATE                     2
#define FILE_OPEN                       3
#define FILE_OPEN_IF                    4
#define FILE_OVERWRITE                  5
#define FILE_OVERWRITE_IF               6

#define FILE_CREATED                    1
#define FILE_OPENED                     2
#define FILE_OVERWRITTEN                3
#define FILE_EXISTS                     4
#define FILE_DOES_NOT_EXIST             5

//
// Change these for their own bitmask values
//

#define FILE_SHARE_READ                 (GENERIC_READ)//0x00000001
#define FILE_SHARE_WRITE                (GENERIC_WRITE)//0x00000002
#define FILE_SHARE_DELETE               (0x00000004)

typedef struct _KSYSTEM_TIME {
    UCHAR Second;
    UCHAR Minute;
    UCHAR Hour;
    UCHAR Day;
    UCHAR Month;
    UCHAR Year;

} KSYSTEM_TIME, *PKSYSTEM_TIME;

typedef struct _PEB {
    // just defining some general pointers for stuff like
    // ntdll & user
    PVOID Pointer;
    PVOID ProcessHeap;
    // brutal.
    UNICODE_STRING CurrentDirectory;
    WCHAR          CurrentDirectoryBuffer[ 256 ]; // :funny_lo:

    // where are you supposed to put this stuff?
    // you gota have gross names for the macros
    PVOID FileStdout;
    PVOID FileStdin;
    PVOID FileStderr;

} PEB, *PPEB;

#define NtCurrentPeb( ) ( ( PPEB )( __readgsqword( 0 ) ) ) 

typedef enum _FILE_INFORMATION_CLASS {
    FileBasicInformation = 1
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

typedef struct _FILE_BASIC_INFORMATION {
    // Ok
    ULONG64 FileLength;
} FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;

#pragma pack(push, 1)

//
// PE Format specific structures.
//

#define IMAGE_DOS_SIGNATURE                 0x5A4D      // MZ
#define IMAGE_NT_SIGNATURE                  0x00004550  // PE00

typedef struct _IMAGE_DOS_HEADER {      // DOS .EXE header
    USHORT   e_magic;                     // Magic number
    USHORT   e_cblp;                      // Buffer on last page of file
    USHORT   e_cp;                        // Pages in file
    USHORT   e_crlc;                      // Relocations
    USHORT   e_cparhdr;                   // Size of header in paragraphs
    USHORT   e_minalloc;                  // Minimum extra paragraphs needed
    USHORT   e_maxalloc;                  // Maximum extra paragraphs needed
    USHORT   e_ss;                        // Initial (relative) SS value
    USHORT   e_sp;                        // Initial SP value
    USHORT   e_csum;                      // Checksum
    USHORT   e_ip;                        // Initial IP value
    USHORT   e_cs;                        // Initial (relative) CS value
    USHORT   e_lfarlc;                    // File address of relocation table
    USHORT   e_ovno;                      // Overlay number
    USHORT   e_res[ 4 ];                    // Reserved USHORTs
    USHORT   e_oemid;                     // OEM identifier (for e_oeminfo)
    USHORT   e_oeminfo;                   // OEM information; e_oemid specific
    USHORT   e_res2[ 10 ];                  // Reserved USHORTs
    ULONG32  e_lfanew;                    // File address of new exe header
} IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

typedef struct _IMAGE_FILE_HEADER {
    USHORT    Machine;
    USHORT    NumberOfSections;
    ULONG32  TimeDateStamp;
    ULONG32  PointerToSymbolTable;
    ULONG32  NumberOfSymbols;
    USHORT    SizeOfOptionalHeader;
    USHORT    Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

#define IMAGE_SIZEOF_FILE_HEADER             20

#define IMAGE_FILE_RELOCS_STRIPPED           0x0001  // Relocation info stripped from file.
#define IMAGE_FILE_EXECUTABLE_IMAGE          0x0002  // File is executable  (i.e. no unresolved externel references).
#define IMAGE_FILE_LINE_NUMS_STRIPPED        0x0004  // Line nunbers stripped from file.
#define IMAGE_FILE_LOCAL_SYMS_STRIPPED       0x0008  // Local symbols stripped from file.
#define IMAGE_FILE_AGGRESIVE_WS_TRIM         0x0010  // Agressively trim working set
#define IMAGE_FILE_LARGE_ADDRESS_AWARE       0x0020  // App can handle >2gb addresses
#define IMAGE_FILE_BYTES_REVERSED_LO         0x0080  // Buffer of machine USHORT are reversed.
#define IMAGE_FILE_32BIT_MACHINE             0x0100  // 32 bit USHORT machine.
#define IMAGE_FILE_DEBUG_STRIPPED            0x0200  // Debugging info stripped from file in .DBG file
#define IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP   0x0400  // If Image is on removable media, copy and run from the swap file.
#define IMAGE_FILE_NET_RUN_FROM_SWAP         0x0800  // If Image is on Net, copy and run from the swap file.
#define IMAGE_FILE_SYSTEM                    0x1000  // System File.
#define IMAGE_FILE_DLL                       0x2000  // File is a DLL.
#define IMAGE_FILE_UP_SYSTEM_ONLY            0x4000  // File should only be run on a UP machine
#define IMAGE_FILE_BYTES_REVERSED_HI         0x8000  // Buffer of machine USHORT are reversed.

#define IMAGE_FILE_MACHINE_UNKNOWN           0
#define IMAGE_FILE_MACHINE_I386              0x014c  // Intel 386.
#define IMAGE_FILE_MACHINE_R3000             0x0162  // MIPS little-endian, 0x160 big-endian
#define IMAGE_FILE_MACHINE_R4000             0x0166  // MIPS little-endian
#define IMAGE_FILE_MACHINE_R10000            0x0168  // MIPS little-endian
#define IMAGE_FILE_MACHINE_WCEMIPSV2         0x0169  // MIPS little-endian WCE v2
#define IMAGE_FILE_MACHINE_ALPHA             0x0184  // Alpha_AXP
#define IMAGE_FILE_MACHINE_SH3               0x01a2  // SH3 little-endian
#define IMAGE_FILE_MACHINE_SH3DSP            0x01a3
#define IMAGE_FILE_MACHINE_SH3E              0x01a4  // SH3E little-endian
#define IMAGE_FILE_MACHINE_SH4               0x01a6  // SH4 little-endian
#define IMAGE_FILE_MACHINE_SH5               0x01a8  // SH5
#define IMAGE_FILE_MACHINE_ARM               0x01c0  // ARM Little-Endian
#define IMAGE_FILE_MACHINE_THUMB             0x01c2
#define IMAGE_FILE_MACHINE_AM33              0x01d3
#define IMAGE_FILE_MACHINE_POWERPC           0x01F0  // IBM PowerPC Little-Endian
#define IMAGE_FILE_MACHINE_POWERPCFP         0x01f1
#define IMAGE_FILE_MACHINE_IA64              0x0200  // Intel 64
#define IMAGE_FILE_MACHINE_MIPS16            0x0266  // MIPS
#define IMAGE_FILE_MACHINE_ALPHA64           0x0284  // ALPHA64
#define IMAGE_FILE_MACHINE_MIPSFPU           0x0366  // MIPS
#define IMAGE_FILE_MACHINE_MIPSFPU16         0x0466  // MIPS
#define IMAGE_FILE_MACHINE_AXP64             IMAGE_FILE_MACHINE_ALPHA64
#define IMAGE_FILE_MACHINE_TRICORE           0x0520  // Infineon
#define IMAGE_FILE_MACHINE_CEF               0x0CEF
#define IMAGE_FILE_MACHINE_EBC               0x0EBC  // EFI Byte Code
#define IMAGE_FILE_MACHINE_AMD64             0x8664  // AMD64 (K8)
#define IMAGE_FILE_MACHINE_M32R              0x9041  // M32R little-endian
#define IMAGE_FILE_MACHINE_CEE               0xC0EE

typedef struct _IMAGE_DATA_DIRECTORY {
    ULONG32  VirtualAddress;
    ULONG32  Size;
} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES    16

typedef struct _IMAGE_OPTIONAL_HEADER64 {
    //
    // Standard fields.
    //

    USHORT  Magic;
    UCHAR   MajorLinkerVersion;
    UCHAR   MinorLinkerVersion;
    ULONG32 SizeOfCode;
    ULONG32 SizeOfInitializedData;
    ULONG32 SizeOfUninitializedData;
    ULONG32 AddressOfEntryPoint;
    ULONG32 BaseOfCode;
    //ULONG32  BaseOfData;

    //
    // NT additional fields.
    //

    ULONG64  ImageBase;
    ULONG32  SectionAlignment;
    ULONG32  FileAlignment;
    USHORT   MajorOperatingSystemVersion;
    USHORT   MinorOperatingSystemVersion;
    USHORT   MajorImageVersion;
    USHORT   MinorImageVersion;
    USHORT   MajorSubsystemVersion;
    USHORT   MinorSubsystemVersion;
    ULONG32  Win32VersionValue;
    ULONG32  SizeOfImage;
    ULONG32  SizeOfHeaders;
    ULONG32  CheckSum;
    USHORT   Subsystem;
    USHORT   DllCharacteristics;
    ULONG64  SizeOfStackReserve;
    ULONG64  SizeOfStackCommit;
    ULONG64  SizeOfHeapReserve;
    ULONG64  SizeOfHeapCommit;
    ULONG32  LoaderFlags;
    ULONG32  NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[ IMAGE_NUMBEROF_DIRECTORY_ENTRIES ];
} IMAGE_OPTIONAL_HEADER64, *PIMAGE_OPTIONAL_HEADER64;

#define IMAGE_NT_OPTIONAL_HDR32_MAGIC      0x10b
#define IMAGE_NT_OPTIONAL_HDR64_MAGIC      0x20b
#define IMAGE_ROM_OPTIONAL_HDR_MAGIC       0x107

typedef struct _IMAGE_NT_HEADERS {
    ULONG32 Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} IMAGE_NT_HEADERS, *PIMAGE_NT_HEADERS;

#define IMAGE_FIRST_SECTION( ntheader ) ((PIMAGE_SECTION_HEADER)\
    ((ULONG64)(ntheader) +\
     FIELD_OFFSET( IMAGE_NT_HEADERS, OptionalHeader ) +\
     ((ntheader))->FileHeader.SizeOfOptionalHeader\
    ))

#define IMAGE_SUBSYSTEM_UNKNOWN              0   // Unknown subsystem.
#define IMAGE_SUBSYSTEM_NATIVE               1   // Image doesn't require a subsystem.
#define IMAGE_SUBSYSTEM_WINDOWS_GUI          2   // Image runs in the Windows GUI subsystem.
#define IMAGE_SUBSYSTEM_WINDOWS_CUI          3   // Image runs in the Windows character subsystem.
#define IMAGE_SUBSYSTEM_OS2_CUI              5   // image runs in the OS/2 character subsystem.
#define IMAGE_SUBSYSTEM_POSIX_CUI            7   // image runs in the Posix character subsystem.
#define IMAGE_SUBSYSTEM_NATIVE_WINDOWS       8   // image is a native Win9x driver.
#define IMAGE_SUBSYSTEM_WINDOWS_CE_GUI       9   // Image runs in the Windows CE subsystem.
#define IMAGE_SUBSYSTEM_EFI_APPLICATION      10  //
#define IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER  11   //
#define IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER   12  //
#define IMAGE_SUBSYSTEM_EFI_ROM              13
#define IMAGE_SUBSYSTEM_XBOX                 14
#define IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION 16

//      IMAGE_LIBRARY_PROCESS_INIT            0x0001     // Reserved.
//      IMAGE_LIBRARY_PROCESS_TERM            0x0002     // Reserved.
//      IMAGE_LIBRARY_THREAD_INIT             0x0004     // Reserved.
//      IMAGE_LIBRARY_THREAD_TERM             0x0008     // Reserved.
#define IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE 0x0040     // DLL can move.
#define IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY    0x0080     // Code Integrity Image
#define IMAGE_DLLCHARACTERISTICS_NX_COMPAT    0x0100     // Image is NX compatible
#define IMAGE_DLLCHARACTERISTICS_NO_ISOLATION 0x0200     // Image understands isolation and doesn't want it
#define IMAGE_DLLCHARACTERISTICS_NO_SEH       0x0400     // Image does not use SEH.  No SE handler may reside in this image
#define IMAGE_DLLCHARACTERISTICS_NO_BIND      0x0800     // Do not bind this image.
//                                            0x1000     // Reserved.
#define IMAGE_DLLCHARACTERISTICS_WDM_DRIVER   0x2000     // Driver uses WDM model
//                                            0x4000     // Reserved.
#define IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE     0x8000

#define IMAGE_DIRECTORY_ENTRY_EXPORT          0   // Export Directory
#define IMAGE_DIRECTORY_ENTRY_IMPORT          1   // Import Directory
#define IMAGE_DIRECTORY_ENTRY_RESOURCE        2   // Resource Directory
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION       3   // Exception Directory
#define IMAGE_DIRECTORY_ENTRY_SECURITY        4   // Security Directory
#define IMAGE_DIRECTORY_ENTRY_BASERELOC       5   // Base Relocation Table
#define IMAGE_DIRECTORY_ENTRY_DEBUG           6   // Debug Directory
//      IMAGE_DIRECTORY_ENTRY_COPYRIGHT       7   // (X86 usage)
#define IMAGE_DIRECTORY_ENTRY_ARCHITECTURE    7   // Architecture Specific Data
#define IMAGE_DIRECTORY_ENTRY_GLOBALPTR       8   // RVA of GP
#define IMAGE_DIRECTORY_ENTRY_TLS             9   // TLS Directory
#define IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG    10   // Load Configuration Directory
#define IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT   11   // Bound Import Directory in headers
#define IMAGE_DIRECTORY_ENTRY_IAT            12   // Import Base Table
#define IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT   13   // Delay Load Import Descriptors
#define IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR 14   // COM Runtime descriptor

#define IMAGE_SIZEOF_SHORT_NAME              8

typedef struct _IMAGE_SECTION_HEADER {
    UCHAR    Name[ IMAGE_SIZEOF_SHORT_NAME ];
    union {
        ULONG32  PhysicalAddress;
        ULONG32  VirtualSize;
    } Misc;
    ULONG32  VirtualAddress;
    ULONG32  SizeOfRawData;
    ULONG32  PointerToRawData;
    ULONG32  PointerToRelocations;
    ULONG32  PointerToLinenumbers;
    USHORT   NumberOfRelocations;
    USHORT   NumberOfLinenumbers;
    ULONG32  Characteristics;
} IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;

#define IMAGE_SIZEOF_SECTION_HEADER          40

//      IMAGE_SCN_TYPE_REG                   0x00000000  // Reserved.
//      IMAGE_SCN_TYPE_DSECT                 0x00000001  // Reserved.
//      IMAGE_SCN_TYPE_NOLOAD                0x00000002  // Reserved.
//      IMAGE_SCN_TYPE_GROUP                 0x00000004  // Reserved.
#define IMAGE_SCN_TYPE_NO_PAD                0x00000008  // Reserved.
//      IMAGE_SCN_TYPE_COPY                  0x00000010  // Reserved.

#define IMAGE_SCN_CNT_CODE                   0x00000020  // Section contains code.
#define IMAGE_SCN_CNT_INITIALIZED_DATA       0x00000040  // Section contains initialized data.
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA     0x00000080  // Section contains uninitialized data.

#define IMAGE_SCN_LNK_OTHER                  0x00000100  // Reserved.
#define IMAGE_SCN_LNK_INFO                   0x00000200  // Section contains comments or some other type of information.
//      IMAGE_SCN_TYPE_OVER                  0x00000400  // Reserved.
#define IMAGE_SCN_LNK_REMOVE                 0x00000800  // Section contents will not become part of image.
#define IMAGE_SCN_LNK_COMDAT                 0x00001000  // Section contents comdat.
//                                           0x00002000  // Reserved.
//      IMAGE_SCN_MEM_PROTECTED - Obsolete   0x00004000
#define IMAGE_SCN_NO_DEFER_SPEC_EXC          0x00004000  // Reset speculative exceptions handling bits in the TLB entries for this section.
#define IMAGE_SCN_GPREL                      0x00008000  // Section content can be accessed relative to GP
#define IMAGE_SCN_MEM_FARDATA                0x00008000
//      IMAGE_SCN_MEM_SYSHEAP  - Obsolete    0x00010000
#define IMAGE_SCN_MEM_PURGEABLE              0x00020000
#define IMAGE_SCN_MEM_16BIT                  0x00020000
#define IMAGE_SCN_MEM_LOCKED                 0x00040000
#define IMAGE_SCN_MEM_PRELOAD                0x00080000

#define IMAGE_SCN_ALIGN_1BYTES               0x00100000  //
#define IMAGE_SCN_ALIGN_2BYTES               0x00200000  //
#define IMAGE_SCN_ALIGN_4BYTES               0x00300000  //
#define IMAGE_SCN_ALIGN_8BYTES               0x00400000  //
#define IMAGE_SCN_ALIGN_16BYTES              0x00500000  // Default alignment if no others are specified.
#define IMAGE_SCN_ALIGN_32BYTES              0x00600000  //
#define IMAGE_SCN_ALIGN_64BYTES              0x00700000  //
#define IMAGE_SCN_ALIGN_128BYTES             0x00800000  //
#define IMAGE_SCN_ALIGN_256BYTES             0x00900000  //
#define IMAGE_SCN_ALIGN_512BYTES             0x00A00000  //
#define IMAGE_SCN_ALIGN_1024BYTES            0x00B00000  //
#define IMAGE_SCN_ALIGN_2048BYTES            0x00C00000  //
#define IMAGE_SCN_ALIGN_4096BYTES            0x00D00000  //
#define IMAGE_SCN_ALIGN_8192BYTES            0x00E00000  //
// Unused                                    0x00F00000
#define IMAGE_SCN_ALIGN_MASK                 0x00F00000

#define IMAGE_SCN_LNK_NRELOC_OVFL            0x01000000  // Section contains extended relocations.
#define IMAGE_SCN_MEM_DISCARDABLE            0x02000000  // Section can be discarded.
#define IMAGE_SCN_MEM_NOT_CACHED             0x04000000  // Section is not cachable.
#define IMAGE_SCN_MEM_NOT_PAGED              0x08000000  // Section is not pageable.
#define IMAGE_SCN_MEM_SHARED                 0x10000000  // Section is shareable.
#define IMAGE_SCN_MEM_EXECUTE                0x20000000  // Section is executable.
#define IMAGE_SCN_MEM_READ                   0x40000000  // Section is readable.
#define IMAGE_SCN_MEM_WRITE                  0x80000000  // Section is writeable.

typedef struct _IMAGE_EXPORT_DIRECTORY {
    ULONG32  Characteristics;
    ULONG32  TimeDateStamp;
    USHORT   MajorVersion;
    USHORT   MinorVersion;
    ULONG32  Name;
    ULONG32  Base;
    ULONG32  NumberOfFunctions;
    ULONG32  NumberOfNames;
    ULONG32  AddressOfFunctions;     // RVA from base of image
    ULONG32  AddressOfNames;         // RVA from base of image
    ULONG32  AddressOfNameOrdinals;  // RVA from base of image
} IMAGE_EXPORT_DIRECTORY, *PIMAGE_EXPORT_DIRECTORY;

typedef struct _IMAGE_IMPORT_BY_NAME {
    USHORT    Hint;
    UCHAR    Name[ 1 ];
} IMAGE_IMPORT_BY_NAME, *PIMAGE_IMPORT_BY_NAME;

typedef struct _IMAGE_THUNK_DATA {
    union {
        ULONG64 ForwarderString;  // PBYTE 
        ULONG64 Function;         // PULONG
        ULONG64 Ordinal;
        ULONG64 AddressOfData;    // PIMAGE_IMPORT_BY_NAME
    } u1;
} IMAGE_THUNK_DATA;
typedef IMAGE_THUNK_DATA * PIMAGE_THUNK_DATA;

#define IMAGE_ORDINAL_FLAG 0x8000000000000000
#define IMAGE_ORDINAL(Ordinal) (Ordinal & 0xffff)
#define IMAGE_SNAP_BY_ORDINAL(Ordinal) ((Ordinal & IMAGE_ORDINAL_FLAG64) != 0)

#if !defined(_MSC_EXTENSIONS)
#define DUMMYUNIONNAME u
#else
#define DUMMYUNIONNAME
#endif

typedef struct _IMAGE_IMPORT_DESCRIPTOR {
    union {
        ULONG32  Characteristics;            // 0 for terminating null import descriptor
        ULONG32  OriginalFirstThunk;         // RVA to original unbound IAT (PIMAGE_THUNK_DATA)
    } DUMMYUNIONNAME;
    ULONG32  TimeDateStamp;                  // 0 if not bound,
                                            // -1 if bound, and real date\time stamp
                                            //     in IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT (new BIND)
                                            // O.W. date/time stamp of DLL bound to (Old BIND)

    ULONG32  ForwarderChain;                 // -1 if no forwarders
    ULONG32  Name;
    ULONG32  FirstThunk;                     // RVA to IAT (if bound this IAT has actual addresses)
} IMAGE_IMPORT_DESCRIPTOR;
typedef IMAGE_IMPORT_DESCRIPTOR *PIMAGE_IMPORT_DESCRIPTOR;

typedef struct _IMAGE_BASE_RELOCATION {
    ULONG32   VirtualAddress;
    ULONG32   SizeOfBlock;
} IMAGE_BASE_RELOCATION;
typedef IMAGE_BASE_RELOCATION UNALIGNED * PIMAGE_BASE_RELOCATION;

#define IMAGE_REL_BASED_ABSOLUTE              0
#define IMAGE_REL_BASED_HIGH                  1
#define IMAGE_REL_BASED_LOW                   2
#define IMAGE_REL_BASED_HIGHLOW               3
#define IMAGE_REL_BASED_HIGHADJ               4
#define IMAGE_REL_BASED_MACHINE_SPECIFIC_5    5
#define IMAGE_REL_BASED_RESERVED              6
#define IMAGE_REL_BASED_MACHINE_SPECIFIC_7    7
#define IMAGE_REL_BASED_MACHINE_SPECIFIC_8    8
#define IMAGE_REL_BASED_MACHINE_SPECIFIC_9    9
#define IMAGE_REL_BASED_DIR64                 10

typedef struct _IMAGE_RUNTIME_FUNCTION {
    ULONG BeginAddress;
    ULONG EndAddress;
    union {
        ULONG UnwindInfoAddress;
        ULONG UnwindData;
    };
} IMAGE_RUNTIME_FUNCTION, *PIMAGE_RUNTIME_FUNCTION;

typedef enum _UNWIND_OP_CODES {
#if 1
    UWOP_PUSH_NONVOL = 0, /* info == register number */
    UWOP_ALLOC_LARGE,     /* no info, alloc size in next 2 slots */
    UWOP_ALLOC_SMALL,     /* info == size of allocation / 8 - 1 */
    UWOP_SET_FPREG,       /* no info, FP = RSP + UNWIND_INFO.FPRegOffset*16 */
    UWOP_SAVE_NONVOL,     /* info == register number, offset in next slot */
    UWOP_SAVE_NONVOL_FAR, /* info == register number, offset in next 2 slots */
    UWOP_EPILOG,
    UWOP_SPARE_CODE,
    UWOP_SAVE_XMM128, /* info == XMM reg number, offset in next slot */
    UWOP_SAVE_XMM128_FAR, /* info == XMM reg number, offset in next 2 slots */
    UWOP_PUSH_MACHFRAME   /* info == 0: no error-code, 1: error-code */
#endif
} UNWIND_CODE_OPS;

typedef union _UNWIND_CODE {
    struct {
        UCHAR CodeOffset;
        UCHAR UnwindOp : 4;
        UCHAR OpInfo : 4;
    };
    USHORT FrameOffset;
} UNWIND_CODE, *PUNWIND_CODE;

#define UNW_FLAG_NHANDLER   0x0
#define UNW_FLAG_EHANDLER   0x1
#define UNW_FLAG_UHANDLER   0x2
#define UNW_FLAG_CHAININFO  0x4

typedef struct _UNWIND_INFO {
    UCHAR Version : 3;
    UCHAR Flags : 5;
    UCHAR SizeOfProlog;
    UCHAR CountOfCodes;
    UCHAR FrameRegister : 4;
    UCHAR FrameOffset : 4;
    UNWIND_CODE UnwindCode[ 1 ];
#if 0
    union {
        //
        // If (Flags & UNW_FLAG_EHANDLER)
        //
        OPTIONAL ULONG ExceptionHandler;
        //
        // Else if (Flags & UNW_FLAG_CHAININFO)
        //
        OPTIONAL ULONG FunctionEntry;
    };
    //
    // If (Flags & UNW_FLAG_EHANDLER)
    //
    OPTIONAL ULONG ExceptionData[ 0 ];
#endif
} UNWIND_INFO, *PUNWIND_INFO;

typedef struct _SCOPE_TABLE {
    ULONG Count;
    struct {
        ULONG BeginAddress;
        ULONG EndAddress;
        ULONG HandlerAddress;
        ULONG JumpTarget;
    } ScopeRecord[ 1 ];
} SCOPE_TABLE, *PSCOPE_TABLE;

#define GetUnwindCodeEntry(info, index) \
    ((info)->UnwindCode[index])

#define GetLanguageSpecificDataPtr(info) \
    ((PVOID)&GetUnwindCodeEntry((info),((info)->CountOfCodes + 1) & ~1))

#define GetExceptionHandler(base, info) \
    ((PEXCEPTION_HANDLER)((base) + *(PULONG)GetLanguageSpecificDataPtr(info)))

#define GetChainedFunctionEntry(base, info) \
    ((PRUNTIME_FUNCTION)((base) + *(PULONG)GetLanguageSpecificDataPtr(info)))

#define GetExceptionDataPtr(info) \
    ((PVOID)((PULONG)GetLanguageSpecificData(info) + 1)

typedef struct _IMAGE_DEBUG_DIRECTORY {
    ULONG   Characteristics;
    ULONG   TimeDateStamp;
    USHORT  MajorVersion;
    USHORT  MinorVersion;
    ULONG   Type;
    ULONG   SizeOfData;
    ULONG   AddressOfRawData;
    ULONG   PointerToRawData;
} IMAGE_DEBUG_DIRECTORY, *PIMAGE_DEBUG_DIRECTORY;

#define IMAGE_DEBUG_TYPE_UNKNOWN          0
#define IMAGE_DEBUG_TYPE_COFF             1
#define IMAGE_DEBUG_TYPE_CODEVIEW         2
#define IMAGE_DEBUG_TYPE_FPO              3
#define IMAGE_DEBUG_TYPE_MISC             4
#define IMAGE_DEBUG_TYPE_EXCEPTION        5
#define IMAGE_DEBUG_TYPE_FIXUP            6
#define IMAGE_DEBUG_TYPE_OMAP_TO_SRC      7
#define IMAGE_DEBUG_TYPE_OMAP_FROM_SRC    8
#define IMAGE_DEBUG_TYPE_BORLAND          9
#define IMAGE_DEBUG_TYPE_RESERVED10       10
#define IMAGE_DEBUG_TYPE_CLSID            11
#define IMAGE_DEBUG_TYPE_VC_FEATURE       12
#define IMAGE_DEBUG_TYPE_POGO             13
#define IMAGE_DEBUG_TYPE_ILTCG            14
#define IMAGE_DEBUG_TYPE_MPX              15
#define IMAGE_DEBUG_TYPE_REPRO            16

#pragma pack(pop)

#define NT_ASSERT_FAIL( e ) __debugbreak( );
#define NT_ASSERT( e ) if ( !( e ) ) { NT_ASSERT_FAIL( L#e ); }

#ifndef __KD_COM__ // kd.h
NTSYSAPI
VOID
KdPrint(
    _In_ PWSTR Format,
    _In_ ...
);
#endif

typedef VOID( *PEXCEPTION_HANDLER )(
    _In_ PEXCEPTION_RECORD Record,
    _In_ PUNWIND_INFO      Unwind
    );
