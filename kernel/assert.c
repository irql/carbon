


#include <carbsup.h>
#include "rtlp.h"
#include "ki.h"


VOID
RtlRaiseAssertionFailure(

)
{

	KeRaiseException( STATUS_ASSERTION_FAILED );
}
