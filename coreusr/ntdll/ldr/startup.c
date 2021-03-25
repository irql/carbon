


#include <carbusr.h>
#include "ldrp.h"


VOID
LdrInitializeProcess(

)
{
    NtCurrentPeb( )->ProcessHeap = RtlCreateHeap( );
    stdin = NULL;
    stdout = NULL;
    stderr = NULL;

}
