


#include <carbusr.h>
#include "../ntdll.h"

char* getenv( const char* name ) {
    name;

    RtlDebugPrint( L"unimplemented getenv: %as\n", name );
    return NULL;// "any:4";
}
