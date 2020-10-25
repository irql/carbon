


#include "com.h"

VOID
KdCmdListThreads(
	__in PKD_CMDR_LIST_THREADS ListThreads
)
{

	ULONG32 ThreadCount = ( ListThreads->Base.KdCmdSize - sizeof( KD_CMDR_LIST_THREADS ) ) / sizeof( KD_THREAD );

	for ( ULONG32 i = 0; i < ThreadCount; i++ ) {

		KdPrint( L"Thread; %d %d\n", ListThreads->Thread[ i ].ProcessId, ListThreads->Thread[ i ].ThreadId );
	}

}