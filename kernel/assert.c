


#include <carbsup.h>
#include "rtlp.h"
#include "ki.h"

DECLSPEC( noreturn )
VOID
RtlRaiseAssertionFailure(

)
{

	KeRaiseException( STATUS_ASSERTION_FAILED );
}
