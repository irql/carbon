


#include "driver.h"
#include "wnd.h"
#include "svga.h"

VOID
NtGdiDiEditboxWndProc(
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

	switch ( Message ) {
	case WM_KEY_PRESS:;

		if ( Window->Flags & EDITBOX_REJECT_INPUT ) {

			break;
		}

		if ( ( WCHAR )Param1 == '\b' ) {

			if ( Window->Name.Length > 0 ) {
				Window->Name.Length--;
				Window->Name.Buffer[ Window->Name.Length ] = ( WCHAR )0;
			}
		}
		else {

			KeAcquireSpinLock( &Window->CompositeLock );
			Window->Name.Buffer[ Window->Name.Length ] = ( WCHAR )Param1;
			Window->Name.Length++;
			Window->Name.Buffer[ Window->Name.Length ] = 0;

			if ( ( Window->Name.Length * sizeof( WCHAR ) ) >= Window->Name.Size ) {

				PWCHAR NewBuffer = MmAllocateMemory( Window->Name.Size + 0x1000, PAGE_READ | PAGE_WRITE );

				_memcpy( NewBuffer, Window->Name.Buffer, Window->Name.Size );

				MmFreeMemory( ( ULONG64 )Window->Name.Buffer, Window->Name.Size );

				Window->Name.Buffer = NewBuffer;
				Window->Name.Size += 0x1000;
			}
			KeReleaseSpinLock( &Window->CompositeLock );
		}

		Window->Parent->Class->WndProc( Window->Parent, WM_COMMAND, Window->MenuId, Param1 );
	case WM_FOCUS:;
	case WM_MOUSE_CLICK:;
		POINT Region = { Window->Rect.right - Window->Rect.left, Window->Rect.bottom - Window->Rect.top };
		POINT EndPoint;

		g_DefaultFont->EndPoint( g_DefaultFont, Window->Name.Buffer, &Region, &EndPoint );

		g_TypingCursorPosition.top = Window->Parent->Rect.top + Window->Rect.top + 3 + EndPoint.y;
		g_TypingCursorPosition.bottom = g_TypingCursorPosition.top + 16;

		g_TypingCursorPosition.left = Window->Parent->Rect.left + Window->Rect.left + EndPoint.x + 3;

		g_CursorVisible = FALSE;
		g_TypingCursor = TRUE;

		SvMoveCursor( FALSE, 0, 0 );
		break;
	default:
		break;
	}
}

VOID
NtGdiDiEditboxClassInit(
	__in PKWINDOW Window,
	__in PWNDCLASSEX Class
)
{

	PDI_EDIT_BOX Editbox = NtGdiAllocateDi( DI_EDIT_BOX );

	Editbox->Base.DrawProcedure = ( DI_DRAW_PROCEDURE )NtGdiDiEditboxDrawProc;
	Editbox->Base.DiFlink = NULL;

	Editbox->Border = Class->Border;
	Editbox->Fill = Class->Fill;

	Window->Name.Buffer = MmAllocateMemory( 0x1000, PAGE_READ | PAGE_WRITE );
	Window->Name.Length = 0;
	Window->Name.Size = 0x1000;
	_memset( Window->Name.Buffer, 0, 0x1000 );

	NtGdiDiInsert( Window, ( PDI_BASE )Editbox );

}

VOID
NtGdiDiEditboxDrawProc(
	__in PKWINDOW Window,
	__in PDI_EDIT_BOX Editbox
)
{
	ULONG32 Width = Window->Rect.right - Window->Rect.left;
	ULONG32 Height = Window->Rect.bottom - Window->Rect.top;

	for ( ULONG32 i = 0; i < ( Width * Height ); i++ ) {

		Window->PrimaryBuffer[ i ] = Editbox->Fill;
	}

	ULONG32 Border = Editbox->Border;
	if ( g_Focus == Window && !( Window->Flags & EDITBOX_NO_FOCUS_OUTLINE ) ) {

		Border = WND_PRIMARY_COLOUR;
	}

	for ( ULONG32 i = 0; i < Width; i++ ) {

		Window->PrimaryBuffer[ i ] = Border;
	}

	for ( ULONG32 i = 0; i < Width; i++ ) {

		Window->PrimaryBuffer[ ( Height - 1 ) * Width + i ] = Border;
	}

	for ( ULONG32 i = 0; i < Height; i++ ) {

		Window->PrimaryBuffer[ Width * i ] = Border;
	}

	for ( ULONG32 i = 0; i < Height; i++ ) {

		Window->PrimaryBuffer[ Width * ( i + 1 ) - 1 ] = Border;
	}

	g_DefaultFont->Render( g_DefaultFont, Window->Name.Buffer, 3, 3, ( ~Editbox->Fill ) | 0xFF000000, Window->PrimaryBuffer, &Window->Rect );

}