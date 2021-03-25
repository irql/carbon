


#include "../carbon.h"
#include "../kd.h"
#include "../kdp.h"
#include "../pdb.h"
#include "../osl/osl.h"

VOID
KdpHandleProcess(
    _In_ PKD_COMMAND Command
)
{

    if ( Command->Arguments[ 1 ].ArgumentType != KdArgumentInteger &&
         Command->Arguments[ 2 ].ArgumentType != KdArgumentEol ) {

        OslWriteConsole( L"syntax error.\n" );
        return;
    }

    KdpProcess = ( PVOID )Command->Arguments[ 1 ].Integer;
}
