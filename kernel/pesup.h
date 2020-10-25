/*++

Module ObjectName:

	pesup.h

Abstract:

	Defines data structures and procedures for PE format binaries.

--*/

#pragma once

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
	USHORT   e_res[4];                    // Reserved USHORTs
	USHORT   e_oemid;                     // OEM identifier (for e_oeminfo)
	USHORT   e_oeminfo;                   // OEM information; e_oemid specific
	USHORT   e_res2[10];                  // Reserved USHORTs
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

typedef struct _IMAGE_OPTIONAL_HEADER {
	//
	// Standard fields.
	//

	USHORT  Magic;
	UCHAR   MajorLinkerVersion;
	UCHAR   MinorLinkerVersion;
	ULONG32  SizeOfCode;
	ULONG32  SizeOfInitializedData;
	ULONG32  SizeOfUninitializedData;
	ULONG32  AddressOfEntryPoint;
	ULONG32  BaseOfCode;
	ULONG32  BaseOfData;

	//
	// NT additional fields.
	//

	ULONG32  ImageBase;
	ULONG32  SectionAlignment;
	ULONG32  FileAlignment;
	USHORT  MajorOperatingSystemVersion;
	USHORT  MinorOperatingSystemVersion;
	USHORT  MajorImageVersion;
	USHORT  MinorImageVersion;
	USHORT  MajorSubsystemVersion;
	USHORT  MinorSubsystemVersion;
	ULONG32  Win32VersionValue;
	ULONG32  SizeOfImage;
	ULONG32  SizeOfHeaders;
	ULONG32  CheckSum;
	USHORT  Subsystem;
	USHORT  DllCharacteristics;
	ULONG32  SizeOfStackReserve;
	ULONG32  SizeOfStackCommit;
	ULONG32  SizeOfHeapReserve;
	ULONG32  SizeOfHeapCommit;
	ULONG32  LoaderFlags;
	ULONG32  NumberOfRvaAndSizes;
	IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER32, *PIMAGE_OPTIONAL_HEADER32;

typedef struct _IMAGE_OPTIONAL_HEADER64 {
	//
	// Standard fields.
	//

	USHORT  Magic;
	UCHAR   MajorLinkerVersion;
	UCHAR   MinorLinkerVersion;
	ULONG32  SizeOfCode;
	ULONG32  SizeOfInitializedData;
	ULONG32  SizeOfUninitializedData;
	ULONG32  AddressOfEntryPoint;
	ULONG32  BaseOfCode;
	//ULONG32  BaseOfData;

	//
	// NT additional fields.
	//

	ULONG64   ImageBase;
	ULONG32    SectionAlignment;
	ULONG32    FileAlignment;
	USHORT    MajorOperatingSystemVersion;
	USHORT    MinorOperatingSystemVersion;
	USHORT    MajorImageVersion;
	USHORT    MinorImageVersion;
	USHORT    MajorSubsystemVersion;
	USHORT    MinorSubsystemVersion;
	ULONG32  Win32VersionValue;
	ULONG32  SizeOfImage;
	ULONG32  SizeOfHeaders;
	ULONG32  CheckSum;
	USHORT    Subsystem;
	USHORT    DllCharacteristics;
	ULONG64   SizeOfStackReserve;
	ULONG64   SizeOfStackCommit;
	ULONG64   SizeOfHeapReserve;
	ULONG64   SizeOfHeapCommit;
	ULONG32  LoaderFlags;
	ULONG32  NumberOfRvaAndSizes;
	IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER64, *PIMAGE_OPTIONAL_HEADER64;

#define IMAGE_NT_OPTIONAL_HDR32_MAGIC      0x10b
#define IMAGE_NT_OPTIONAL_HDR64_MAGIC      0x20b
#define IMAGE_ROM_OPTIONAL_HDR_MAGIC       0x107

#undef PIMAGE_NT_HEADERS32

typedef struct _IMAGE_NT_HEADERS32 {
	ULONG32 Signature;
	IMAGE_FILE_HEADER FileHeader;
	IMAGE_OPTIONAL_HEADER32 OptionalHeader;
} IMAGE_NT_HEADERS32;// , *PIMAGE_NT_HEADERS32;

typedef struct _IMAGE_NT_HEADERS64 {
	ULONG32 Signature;
	IMAGE_FILE_HEADER FileHeader;
	IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} IMAGE_NT_HEADERS64;// , *PIMAGE_NT_HEADERS64;

#ifdef _WIN64
typedef IMAGE_NT_HEADERS64                  IMAGE_NT_HEADERS;
typedef IMAGE_NT_HEADERS64                 *PIMAGE_NT_HEADERS;
#else
typedef IMAGE_NT_HEADERS32                  IMAGE_NT_HEADERS;
typedef IMAGE_NT_HEADERS32                 *PIMAGE_NT_HEADERS;
#endif

#ifdef _WIN64
#define IMAGE_FIRST_SECTION IMAGE_FIRST_SECTION64
#else
#define IMAGE_FIRST_SECTION IMAGE_FIRST_SECTION32
#endif

#define IMAGE_FIRST_SECTION64( ntheader ) ((PIMAGE_SECTION_HEADER)\
    ((ULONG64)(ntheader) +\
     FIELD_OFFSET( IMAGE_NT_HEADERS64, OptionalHeader ) +\
     ((ntheader))->FileHeader.SizeOfOptionalHeader\
    ))

#define IMAGE_FIRST_SECTION32( ntheader ) ((PIMAGE_SECTION_HEADER)\
    ((ULONG)(ntheader) +\
     FIELD_OFFSET( IMAGE_NT_HEADERS32, OptionalHeader ) +\
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
	UCHAR    Name[IMAGE_SIZEOF_SHORT_NAME];
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
	USHORT    MajorVersion;
	USHORT    MinorVersion;
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
	UCHAR    Name[1];
} IMAGE_IMPORT_BY_NAME, *PIMAGE_IMPORT_BY_NAME;

typedef struct _IMAGE_THUNK_DATA64 {
	union {
		ULONG64 ForwarderString;  // PBYTE 
		ULONG64 Function;         // PULONG
		ULONG64 Ordinal;
		ULONG64 AddressOfData;    // PIMAGE_IMPORT_BY_NAME
	} u1;
} IMAGE_THUNK_DATA64;
typedef IMAGE_THUNK_DATA64 * PIMAGE_THUNK_DATA64;

typedef struct _IMAGE_THUNK_DATA32 {
	union {
		ULONG32 ForwarderString;      // PBYTE 
		ULONG32 Function;             // PULONG
		ULONG32 Ordinal;
		ULONG32 AddressOfData;        // PIMAGE_IMPORT_BY_NAME
	} u1;
} IMAGE_THUNK_DATA32;
typedef IMAGE_THUNK_DATA32 *PIMAGE_THUNK_DATA32;

#ifdef _WIN64
typedef IMAGE_THUNK_DATA64                  IMAGE_THUNK_DATA;
typedef PIMAGE_THUNK_DATA64                 PIMAGE_THUNK_DATA;
#else
typedef IMAGE_THUNK_DATA32                  IMAGE_THUNK_DATA;
typedef PIMAGE_THUNK_DATA32                 PIMAGE_THUNK_DATA;
#endif

#define IMAGE_ORDINAL_FLAG64 0x8000000000000000
#define IMAGE_ORDINAL_FLAG32 0x80000000
#define IMAGE_ORDINAL64(Ordinal) (Ordinal & 0xffff)
#define IMAGE_ORDINAL32(Ordinal) (Ordinal & 0xffff)
#define IMAGE_SNAP_BY_ORDINAL64(Ordinal) ((Ordinal & IMAGE_ORDINAL_FLAG64) != 0)
#define IMAGE_SNAP_BY_ORDINAL32(Ordinal) ((Ordinal & IMAGE_ORDINAL_FLAG32) != 0)

typedef struct _IMAGE_IMPORT_DESCRIPTOR {
	union {
		ULONG32  Characteristics;            // 0 for terminating null import descriptor
		ULONG32  OriginalFirstThunk;         // RVA to original unbound IAT (PIMAGE_THUNK_DATA)
	};
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

FORCEINLINE
BOOLEAN
PeSupVerifyDosHeader(
	__in PIMAGE_DOS_HEADER DosHeader
)
{
	if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE) {

		return FALSE;
	}

	return TRUE;
}

FORCEINLINE
BOOLEAN
PeSupVerifyNtHeaders(
	__in IMAGE_NT_HEADERS* NtHeaders
)
{
	if (NtHeaders->Signature != IMAGE_NT_SIGNATURE) {

		return FALSE;
	}

	return TRUE;
}


NTSTATUS
PeSupGetProcedureAddressByName(
	__in PVOID ModuleBase,
	__in CHAR* ExportName,
	__out PVOID* ProcedureAddress
	);


NTSTATUS
PeSupGetProcedureAddressByOrdinal(
	__in PVOID ModuleBase,
	__in USHORT ExportOrdinal,
	__out PVOID* ProcedureAddress
	);

NTSTATUS
PeSupResolveImportDescriptorSingle(
	__in PVOID ModuleBase,
	__in PVOID DescribedModuleBase,
	__in PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor
	);

NTSTATUS
PeSupResolveBaseRelocDescriptor(
	__in PVOID ModuleBase,
	__in PIMAGE_BASE_RELOCATION BaseRelocation
	);