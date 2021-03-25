


#include <carbsup.h>
#include "mi.h"
#include "../hal/halp.h"
#include "../ke/ki.h"

PMM_VAD
MiAllocateVad(

)
{
    return MmAllocatePoolWithTag( NonPagedPoolZeroed, sizeof( MM_VAD ), MM_TAG );
}

VOID
MiFreeVad(
    _In_ PMM_VAD Vad
)
{
    MmFreePoolWithTag( Vad, MM_TAG );
}

VOID
MiInsertVad(
    _In_ PKPROCESS Process,
    _In_ PMM_VAD   Vad
)
{
    PMM_VAD Final;

    Vad->Link = NULL;
    if ( Process->VadRoot == NULL ) {

        Process->VadRoot = Vad;

        return;
    }

    Final = Process->VadRoot;

    while ( Final->Link != NULL ) {

        Final = Final->Link;
    }

    Final->Link = Vad;
}

PMM_VAD
MiFindVadByFullName(
    _In_ PKPROCESS       Process,
    _In_ PUNICODE_STRING FileName
)
{
    PMM_VAD CurrentVad;

    if ( Process->VadRoot == NULL ) {

        return NULL;
    }

    CurrentVad = Process->VadRoot;

    do {
        if ( RtlCompareUnicodeString( &CurrentVad->FileObject->FileName, FileName, FALSE ) == 0 ) {

            return CurrentVad;
        }

        CurrentVad = CurrentVad->Link;
    } while ( CurrentVad != NULL );

    return NULL;
}

PMM_VAD
MiFindVadByShortName(
    _In_ PKPROCESS       Process,
    _In_ PUNICODE_STRING ShortName
)
{
    PMM_VAD CurrentVad;

    if ( Process->VadRoot == NULL ) {

        return NULL;
    }

    // might wana sync this
    CurrentVad = Process->VadRoot;
    do {
        if ( RtlCompareString( ShortName->Buffer,
                               CurrentVad->FileObject->FileName.Buffer +
                               FsRtlFileNameIndex( &CurrentVad->FileObject->FileName ),
                               TRUE ) == 0 ) {

            return CurrentVad;
        }

        CurrentVad = CurrentVad->Link;
    } while ( CurrentVad != NULL );

    return NULL;
}

PMM_VAD
MiFindVadByAddress(
    _In_ PKPROCESS Process,
    _In_ ULONG64   Address
)
{
    PMM_VAD CurrentVad;

    if ( Process->VadRoot == NULL ) {

        return NULL;
    }

    CurrentVad = Process->VadRoot;
    do {
        if ( Address >= CurrentVad->Start &&
             Address < CurrentVad->End ) {

            return CurrentVad;
        }

        CurrentVad = CurrentVad->Link;
    } while ( CurrentVad != NULL );

    return NULL;
}

VOID
MiRemoveVadByPointer(
    _In_ PKPROCESS Process,
    _In_ PMM_VAD   Vad
)
{
    PMM_VAD CurrentVad;
    PMM_VAD LastVad;

    LastVad = Process->VadRoot;
    CurrentVad = Process->VadRoot;

    while ( CurrentVad != NULL ) {

        if ( CurrentVad == Vad ) {
            if ( Process->VadRoot == CurrentVad ) {
                Process->VadRoot = CurrentVad->Link;
            }
            else {
                LastVad->Link = CurrentVad->Link;
            }
        }

        LastVad = CurrentVad;
        CurrentVad = CurrentVad->Link;
    }

    return;
}
