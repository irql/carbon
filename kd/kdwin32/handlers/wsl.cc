


#include "../carbon.h"
#include "../kd.h"
#include "../kdp.h"
#include "../pdb.h"
#include "../osl/osl.h"

// add pfn display too
VOID
KdpHandleWsl(
    _In_ PKD_COMMAND Command
)
{

    MM_WSL CurrentWsl;
    ULONG64 CurrentWsle;
    ULONG64 CurrentWslAddress;
    ULONG64 CurrentWsleTotal;
    PVOID PreviousProcess;

    switch ( Command->Arguments[ 0 ].String[ 3 ] ) {
    case 'p': // display process working set

        if ( Command->ArgumentCount != 3 ||
             Command->Arguments[ 1 ].ArgumentType != KdArgumentInteger ||
             Command->Arguments[ 2 ].ArgumentType != KdArgumentEol ) {

            OslWriteConsole( L"syntax error.\n" );
            break;
        }

        //
        // need to improve on memory read/write apis, the proper way
        // to test for if a wsl table is valid is by ptes
        //

        CurrentWslAddress = 0xFFFFFE0000001000;
        CurrentWsleTotal = 0;
        do {

            CurrentWsl.WorkingSetListCount = 0;

            PreviousProcess = KdpProcess;
            KdpProcess = ( PVOID )Command->Arguments[ 1 ].Integer;
            if ( KdpReadDebuggee( CurrentWslAddress, 0x1000, &CurrentWsl ) != KdStatusSuccess ) {

                KdpProcess = PreviousProcess;
                break;
            }
            KdpProcess = PreviousProcess;

            for ( CurrentWsle = 0; CurrentWsle < 255; CurrentWsle++, CurrentWsleTotal++ ) {

                if ( CurrentWsl.WorkingSetList[ CurrentWsle ].Usage != MmMappedUnused ) {

                    switch ( CurrentWsl.WorkingSetList[ CurrentWsle ].Usage ) {
                    case MmMappedPhysical:
                        OslWriteConsole( L"[%04d] MmMappedPhysical: Address: 0x%p PFN: 0x%p\n",
                                         CurrentWsleTotal,
                                         CurrentWsl.WorkingSetList[ CurrentWsle ].TypeMappedPhysical.Address,
                                         CurrentWsl.WorkingSetList[ CurrentWsle ].TypeMappedPhysical.IndexPfn );
                        break;
                    case MmMappedViewOfSection:
                        OslWriteConsole( L"[%04d] MmMappedViewOfSection: Address: 0x%p Section: 0x%p Length: 0x%p\n",
                                         CurrentWsleTotal,
                                         CurrentWsl.WorkingSetList[ CurrentWsle ].TypeMappedViewOfSection.Address << 12,
                                         CurrentWsl.WorkingSetList[ CurrentWsle ].TypeMappedViewOfSection.SectionObject | 0xFFFF000000000000,
                                         ( CurrentWsl.WorkingSetList[ CurrentWsle ].TypeMappedViewOfSection.LengthLower |
                                         ( CurrentWsl.WorkingSetList[ CurrentWsle ].TypeMappedViewOfSection.LengthUpper << 8 ) ) << 12 );
                        break;
                    default:

                        break;
                    }
                }
            }

            CurrentWslAddress += 0x1000;

        } while ( CurrentWsl.WorkingSetListCount > 0 );

        break;
    case 'e': // display wsl entry by address
    default:
        OslWriteConsole( L"unrecognised format.\n" );
        break;
    }
}
