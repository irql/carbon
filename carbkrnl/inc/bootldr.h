


#pragma once

#pragma pack(push, 1)
typedef struct _MEMORY_MAP_ENTRY {
    ULONG64 RegionBase;
    ULONG64 RegionLength;
    ULONG32 RegionType;
    ULONG32 AcpiEa;
} MEMORY_MAP_ENTRY, *PMEMORY_MAP_ENTRY;

typedef struct _MEMORY_MAP {
    ULONG32             EntryCount;
    MEMORY_MAP_ENTRY    Entries[ 0 ];
} MEMORY_MAP, *PMEMORY_MAP;

typedef struct _RD_FILE_LIST_ENTRY {
    CHAR    DirectoryFile[ 12 ];
    ULONG32 FilePointer;
    ULONG32 Length;
} RD_FILE_LIST_ENTRY, *PRD_FILE_LIST_ENTRY;

typedef struct _RD_FILE_LIST {
    ULONG32             EntryCount;
    RD_FILE_LIST_ENTRY  Entries[ 0 ];
} RD_FILE_LIST, *PRD_FILE_LIST;

typedef struct _LDR_BOOT_INFO {
    ULONG32         DisplayWidth;
    ULONG32         DisplayHeight;
    ULONG32         BitsPerPixel;
    ULONG32         Framebuffer;
    ULONG32         RootSerial;
    //PMEMORY_MAP     MemoryMap;
    ULONG32         MemoryMap;
    //PRD_FILE_LIST   FileList;
    ULONG32         FileList;
} LDR_BOOT_INFO, *PLDR_BOOT_INFO;
#pragma pack(pop)