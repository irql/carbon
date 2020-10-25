


#include <carbsup.h>
#include "mm.h"

VOID
MiInvalidateTlbEntry(
	__in ULONG64 Address
)
{
	__invlpg( ( void* )Address );

	/*	can you implement a kernel apc queue,
		so that i can queue these, interrupts would be slow and ass,
		on this note, can you please create something similar to
		KeSaveFloatingPointState which should write to the current
		thread's TCB */
}