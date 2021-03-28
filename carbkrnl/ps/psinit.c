


#include <carbsup.h>
#include "psp.h"
#include "../hal/halp.h"
#include "../ke/ki.h"
#include "../ob/obp.h"
#include "../mm/mi.h"
#include "../io/iop.h"

VOID
PsInitializeProcessSystem(

)
{
    STATIC OBJECT_ATTRIBUTES SystemAttributes = { { 0 }, { 0 }, OBJ_PERMANENT_OBJECT };

    //PIMAGE_DOS_HEADER HeaderDos;
    //PIMAGE_NT_HEADERS HeadersNt;
    //PIMAGE_SECTION_HEADER LastSection;
    PKPROCESS Process;

    ObCreateObject( &Process,
                    PsProcessObject,
                    &SystemAttributes,
                    sizeof( KPROCESS ) );
#if 0
    Process->VadRoot = MiAllocateVad( );
    Process->VadRoot->Start = 0xFFFFFFFFFFE00000;

    HeaderDos = ( PIMAGE_DOS_HEADER )( 0xFFFFFFFFFFE00000 );
    HeadersNt = ( PIMAGE_NT_HEADERS )( 0xFFFFFFFFFFE00000 + HeaderDos->e_lfanew );
    LastSection = &IMAGE_FIRST_SECTION( HeadersNt )[ HeadersNt->FileHeader.NumberOfSections - 1 ];

    Process->VadRoot->End = Process->VadRoot->Start + ( ULONG64 )LastSection->VirtualAddress + ROUND_TO_PAGES( LastSection->Misc.VirtualSize );
    //Process->VadRoot->FileObject = IopSystemFileObject;
#endif
    Process->DirectoryTableBase = __readcr3( );
    Process->VadRoot = NULL;
    Process->ProcessId = KeGenerateUniqueId( );
    KeInitializeHeadList( &Process->ProcessLinks );
    PsInitialSystemProcess = Process;

}
