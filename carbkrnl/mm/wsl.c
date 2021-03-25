


#include <carbsup.h>
#include "mi.h"

PMM_WSL MmCurrentWorkingSetList = ( PMM_WSL )( 0xFFFFFE0000001000 );

VOID
MmInsertWorkingSet(
    _In_ PMM_WSLE  Entry
)
{
    PPMLE   CurrentWslPageTable;
    PMM_WSL CurrentWsl;
    BOOLEAN Zeroed;
    ULONG64 CurrentWsle;

    CurrentWsl = ( PMM_WSL )( ( PUCHAR )MmCurrentWorkingSetList - 0x1000 );

    do {

        CurrentWsl = ( PMM_WSL )( ( PUCHAR )CurrentWsl + 0x1000 );
        CurrentWslPageTable = MmAddressPageTable( ( ULONG64 )CurrentWsl );

        if ( !CurrentWslPageTable[ MiIndexLevel1( CurrentWsl ) ].Present ) {
            CurrentWslPageTable[ MiIndexLevel1( CurrentWsl ) ].PageFrameNumber = MmAllocateZeroedPhysical( MmTypeWorkingSetList, &Zeroed ) >> 12;
            CurrentWslPageTable[ MiIndexLevel1( CurrentWsl ) ].Write = 1;
            CurrentWslPageTable[ MiIndexLevel1( CurrentWsl ) ].Present = 1;
            if ( !Zeroed ) {
                RtlZeroMemory( CurrentWsl, 0x1000 );
            }
            break;
        }

    } while ( CurrentWsl->WorkingSetListCount == 255 );

    for ( CurrentWsle = 0; CurrentWsle < 255; CurrentWsle++ ) {

        if ( CurrentWsl->WorkingSetList[ CurrentWsle ].Usage == MmMappedUnused ) {
            CurrentWsl->WorkingSetList[ CurrentWsle ].Upper = Entry->Upper;
            CurrentWsl->WorkingSetList[ CurrentWsle ].Lower = Entry->Lower;
            CurrentWsl->WorkingSetListCount++;
            return;
        }
    }
}

PMM_WSLE
MmFindWorkingSetByAddress(
    _In_ MM_WSLE_USE Usage,
    _In_ ULONG64     Address
)
{

    PPMLE   CurrentWslPageTable;
    PMM_WSL CurrentWsl;
    ULONG64 CurrentWsle;

    CurrentWsl = MmCurrentWorkingSetList;
    CurrentWslPageTable = MmAddressPageTable( ( ULONG64 )CurrentWsl );

    while ( CurrentWslPageTable[ MiIndexLevel1( CurrentWsl ) ].Present ) {

        if ( CurrentWsl->WorkingSetListCount > 0 ) {

            for ( CurrentWsle = 0; CurrentWsle < 255; CurrentWsle++ ) {

                if ( CurrentWsl->WorkingSetList[ CurrentWsle ].Usage != Usage ) {
                    continue;
                }

                switch ( Usage ) {
                case MmMappedPhysical:
                    if ( CurrentWsl->WorkingSetList[ CurrentWsle ].TypeMappedPhysical.Address == Address ) {
                        return &CurrentWsl->WorkingSetList[ CurrentWsle ];
                    }
                    break;
                case MmMappedViewOfSection:
                    if ( ( CurrentWsl->WorkingSetList[ CurrentWsle ].TypeMappedViewOfSection.Address << 12 ) == ( Address & 0xFFFFFFFFFFFF ) ) {
                        return &CurrentWsl->WorkingSetList[ CurrentWsle ];
                    }
                    break;
                default:
                    return NULL;
                }
            }
        }

        CurrentWsl = ( PMM_WSL )( ( PUCHAR )CurrentWsl + 0x1000 );
        CurrentWslPageTable = MmAddressPageTable( ( ULONG64 )CurrentWsl );
    }

    return NULL;
}


VOID
MmFreeWorkingSetListEntry(
    _In_ PMM_WSLE Entry
)
{
    PMM_WSL Wsl;

    Entry->Upper = 0;
    Entry->Lower = 0;

    Wsl = ( PMM_WSL )( ( ULONG64 )Entry & ~0xFFF );
    Wsl->WorkingSetListCount--;
}
