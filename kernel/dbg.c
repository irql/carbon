


#include <carbsup.h>

KSPIN_LOCK DbgLineLock = { 0 };
PLIST_ENTRY DbgLineHeader = NULL;

VOID
DbgPrint(
	__in PWSTR Format,
	__in ...
)
{
	STATIC ULONG32 LineNumber = 0;

	/* Implement a max buffer parameter for a vsprintfw */

	PDBG_LINE DbgLine = ( PDBG_LINE )ExAllocatePoolWithTag( sizeof( DBG_LINE ), TAGEX_DEBUG );
	DbgLine->LineNumber = LineNumber++;

	va_list Args;
	va_start( Args, Format );
	vsprintfW( ( wchar_t* )&DbgLine->LineBuffer, Format, Args );
	va_end( Args );

	printf( "%w", DbgLine->LineBuffer );

	KeAcquireSpinLock( &DbgLineLock );

	if ( DbgLineHeader == NULL ) {
		KeInitializeListHead( &DbgLine->LineLinks );
		DbgLineHeader = &DbgLine->LineLinks;
	}
	else {

		KeInsertListEntry( DbgLineHeader, &DbgLine->LineLinks );
	}

	KeReleaseSpinLock( &DbgLineLock );
}
