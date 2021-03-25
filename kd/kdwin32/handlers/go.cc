


#include "../carbon.h"
#include "../kd.h"
#include "../kdp.h"
#include "../pdb.h"
#include "../osl/osl.h"

VOID
KdpHandleContinue(
    _In_ PKD_COMMAND Command
)
{
    if ( Command->Arguments[ 1 ].ArgumentType != KdArgumentEol ) {

        OslWriteConsole( L"expected eol\n" );
        return;
    }

    KdpContinue( );
}
