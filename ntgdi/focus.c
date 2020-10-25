


#include "driver.h"
#include "wnd.h"

PKWINDOW g_Focus;

PKWINDOW
NtGdiWindowFromPoint(
	__in PPOINT Point
)
{

	KeAcquireSpinLock( &g_WindowListLock );

	PKWINDOW Last = g_RootWindow;

	while ( Last->Next != NULL ) {

		Last = Last->Next;
	}

	PKWINDOW Window;

	while ( Last != g_RootWindow ) {
		Window = Last;

		Last = g_RootWindow;
		while ( Last->Next != Window ) {

			Last = Last->Next;
		}

		//KdCmdMessage( L"checking %s", Window->Name.Buffer );

		if ( Window == g_RootWindow ) {

			continue;
		}

		if ( Window->Rect.top > Point->y ||
			Window->Rect.bottom < Point->y ) {

			continue;
		}

		if ( Window->Rect.left > Point->x ||
			Window->Rect.right < Point->x ) {

			continue;
		}

		//KdCmdMessage( L"success %s", Window->Name.Buffer );

		//
		//	now that we know this point is inside the client area
		//	we can start scanning the child windows.
		//

		PKWINDOW WindowPoint = Window;
		PKWINDOW ChildWindow = Window->Child;

		while ( ChildWindow != NULL ) {

			if ( ( Window->Rect.top + ChildWindow->Rect.top ) > Point->y ||
				( Window->Rect.top + ChildWindow->Rect.bottom ) < Point->y ) {

				ChildWindow = ChildWindow->Child;
				continue;
			}

			if ( ( Window->Rect.left + ChildWindow->Rect.left ) > Point->x ||
				( Window->Rect.left + ChildWindow->Rect.right ) < Point->x ) {

				ChildWindow = ChildWindow->Child;
				continue;
			}

			//KdCmdMessage( L"pd %s", ChildWindow->Name.Buffer );

			WindowPoint = ChildWindow;
			ChildWindow = ChildWindow->Child;
		}

		KeReleaseSpinLock( &g_WindowListLock );
		return WindowPoint;
	}

	KeReleaseSpinLock( &g_WindowListLock );

	return NULL;
}
