

#include "driver.h"
#include "wnd.h"

VOID
NtGdiMouseHandleClick(
	__in PMOUSE_PACKET MousePacket
)
{

	STATIC POINT PressPosition = { 0 };
	STATIC POINT MovedPosition = { 0 };
	STATIC BOOLEAN PressLock = 0;
	STATIC PKWINDOW PressWindow = NULL;

	if ( !PressLock ) {

		PressWindow = NtGdiWindowFromPoint( &MousePacket->Position );

		PressPosition.x = MousePacket->Position.x;
		PressPosition.y = MousePacket->Position.y;
		MovedPosition.x = MousePacket->Position.x;
		MovedPosition.y = MousePacket->Position.y;

		PressLock = TRUE;

		if ( PressWindow != NULL && PressWindow->Parent != g_RootWindow && PressWindow != g_Focus ) {

			g_Focus = PressWindow;

			//KdCmdMessage( L"new focus %s @ %d %d", PressWindow->Parent->Name.Buffer, PressPosition.x, PressPosition.y );

			PressWindow->Parent->Class->WndProc( PressWindow->Parent, WM_FOCUS, 0, 0 );

			KeAcquireSpinLock( &g_WindowListLock );
			NtGdiWindowRemove( PressWindow->Parent );
			NtGdiWindowInsert( PressWindow->Parent );
			KeReleaseSpinLock( &g_WindowListLock );

			if ( PressWindow->Parent != PressWindow ) {

				PressWindow->Class->WndProc( PressWindow, WM_FOCUS, 0, 0 );
			}
		}
	}

	if ( PressWindow == NULL || PressWindow->Parent == g_RootWindow || PressWindow != g_Focus ) {

		return;
	}

	if ( MousePacket->State == KeyStatePress ) {

		MovedPosition.x = MousePacket->Position.x;
		MovedPosition.y = MousePacket->Position.y;
	}

	if ( MousePacket->State == KeyStateRelease ) {

		PKWINDOW HoverWindow = NtGdiWindowFromPoint( &MousePacket->Position );

		if ( PressWindow->Parent == PressWindow &&
			PressPosition.y < PressWindow->Rect.top + WND_TITLE_BAR_H ) {

			PressWindow->Rect.left += ( MovedPosition.x - PressPosition.x );
			PressWindow->Rect.right += ( MovedPosition.x - PressPosition.x );

			PressWindow->Rect.top += ( MovedPosition.y - PressPosition.y );
			PressWindow->Rect.bottom += ( MovedPosition.y - PressPosition.y );
		}

		if ( PressWindow == HoverWindow ) {

			//KdCmdMessage( L"sent mouse_click to %s", PressWindow->Name.Buffer );
			PressWindow->Class->WndProc( PressWindow, WM_MOUSE_CLICK, 0, 0 );
		}

		PressLock = FALSE;
	}
}

VOID
NtGdiMouseHandleMove(
	__in POINT OldPosition,
	__in POINT NewPosition
)
{
	OldPosition;
	NewPosition;
}

VOID
NtGdiWndProcDefault(
	__in PKWINDOW Window,
	__in ULONG32 Message,
	__in ULONG32 Param1,
	__in ULONG32 Param2
)
{
	Window;
	Message;
	Param1;
	Param2;

	return;
}
