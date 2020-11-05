


#include "driver.h"

PLIST_ENTRY VerboseHead = NULL;

PVERBOSE_ENTRY
NtGdiVerboseAddEntry(
	__in PUNICODE_STRING String
)
{
	PVERBOSE_ENTRY VerboseEntry = ExAllocatePoolWithTag( sizeof( VERBOSE_ENTRY ), 'BREV' );

	if ( VerboseHead == NULL ) {

		KeInitializeListHead( &VerboseEntry->VerboseLinks );
		VerboseHead = &VerboseEntry->VerboseLinks;
	}
	else {

		KeInsertListEntry( VerboseHead, &VerboseEntry->VerboseLinks );
	}

	VerboseEntry->VerboseString = String;

	return VerboseEntry;
}

VOID
NtGdiVerboseOverlayRender(

)
{

	if ( VerboseHead == NULL ) {

		return;
	}

	ULONG32 x = 0, y = 0;

	RECT Region = { 0, 0, g_Basic.Width, g_Basic.Height };
	POINT Region1 = { g_Basic.Width, g_Basic.Height };
	POINT ExtentPoint;

	PLIST_ENTRY Flink = VerboseHead;
	do {
		PVERBOSE_ENTRY Entry = CONTAINING_RECORD( Flink, VERBOSE_ENTRY, VerboseLinks );

		g_DefaultFont->ExtentPoint( g_DefaultFont, Entry->VerboseString->Buffer, &Region1, &ExtentPoint );

		POINT pos = { x, y };
		while ( pos.y <= y + ExtentPoint.y + 16 ) {

			NtGdiPutPixel( g_Basic.Doublebuffer, pos.x, pos.y, 0xFF000000 );

			if ( pos.x >= x + ExtentPoint.x ) {
				pos.x = x;
				pos.y++;
			}
			else {

				pos.x++;
			}
		}


		g_DefaultFont->Render( g_DefaultFont, Entry->VerboseString->Buffer, x, y, 0xFFFFFFFF, g_Basic.Doublebuffer, &Region );

		x += ( ExtentPoint.x );
		y += ( ExtentPoint.y ) + 16;
		x = 0;


		Flink = Flink->Flink;
	} while ( Flink != VerboseHead );

}