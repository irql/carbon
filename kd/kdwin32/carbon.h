


#pragma once

//#define KRNLINTERNAL

#define __CARBON_H__

#ifdef __cplusplus
#define EXTERN_C        extern "C"
#else
#define EXTERN_C        extern
#endif
#define NTSYSAPI        EXTERN_C

#include <carbsup.h>

typedef struct _MM_VAD {
    PMM_VAD         Link;
    ULONG64         Start;
    ULONG64         End;
    ULONG64         Charge;
    PIO_FILE_OBJECT FileObject;
} MM_VAD, *PMM_VAD;

typedef enum _MM_WSLE_USE {
    MmMappedUnused,
    MmMappedPhysical,
    MmMappedViewOfSection,
    MmMappedMaximum
} MM_WSLE_USE;

typedef struct _MM_WSLE {
    union {
        struct {
            ULONG64 Usage : 8;
            ULONG64 Ignore : 56;
        };

        struct {
            ULONG64 Usage : 8;
            ULONG64 IndexPfn : 56;
            ULONG64 Address;
        } TypeMappedPhysical;

        struct {
            ULONG64 Usage : 8;
            ULONG64 SectionObject : 48;
            ULONG64 LengthLower : 8;
            ULONG64 LengthUpper : 28;
            ULONG64 Address : 36;
        } TypeMappedViewOfSection;

        struct {
            ULONG64 Upper;
            ULONG64 Lower;
        };
    };
} MM_WSLE, *PMM_WSLE;

C_ASSERT( sizeof( MM_WSLE ) == 16 );

typedef struct _MM_WSL {
    ULONG64 WorkingSetListCount;
    MM_WSLE WorkingSetList[ 255 ];
} MM_WSL, *PMM_WSL;

C_ASSERT( sizeof( MM_WSL ) <= 4096 );
