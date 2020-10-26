

#include "driver.h"
#include "wnd.h"

VOID
NtGdiWindowComposite(
	__in PKWINDOW WindowObject
)
{

	KeAcquireSpinLock( &WindowObject->CompositeLock );

	PDI_BASE DiFlink = WindowObject->DiFlink;
	while ( DiFlink != NULL ) {

		DiFlink->DrawProcedure( WindowObject, DiFlink );
		DiFlink = DiFlink->DiFlink;
	}

	KeReleaseSpinLock( &WindowObject->CompositeLock );

	if ( WindowObject->Child != NULL ) {

		NtGdiWindowComposite( WindowObject->Child );

		ULONG32 Width = WindowObject->Child->Rect.right - WindowObject->Child->Rect.left;
		ULONG32 Height = WindowObject->Child->Rect.bottom - WindowObject->Child->Rect.top;

		POINT CurrentPixel = { 0 };
		for ( ULONG32 i = 0; i < ( Width * Height ); i++ ) {

			NtGdiPutPixel2( WindowObject->Parent->PrimaryBuffer,
				WindowObject->Child->Rect.left + CurrentPixel.x,
				WindowObject->Child->Rect.top + CurrentPixel.y,
				WindowObject->Child->PrimaryBuffer[ i ],
				WindowObject->Parent->Rect.right - WindowObject->Parent->Rect.left,
				WindowObject->Parent->Rect.bottom - WindowObject->Parent->Rect.top );

			CurrentPixel.x++;

			if ( CurrentPixel.x >= Width ) {

				CurrentPixel.y++;
				CurrentPixel.x = 0;
			}
		}

	}
}



