

#include "driver.h"
#include "wnd.h"

VOID
NtGdiDiWindowBaseClassInit(
	__in PKWINDOW Window,
	__in PWNDCLASSEX Class
)
{

	PDI_WINDOW_BASE WindowBase = NtGdiAllocateDi( DI_WINDOW_BASE );

	WindowBase->Base.DrawProcedure = ( DI_DRAW_PROCEDURE )NtGdiDiWindowBaseDrawProc;
	WindowBase->Base.DiFlink = NULL;
	WindowBase->Border = Class->Border;
	WindowBase->Fill = Class->Fill;
	WindowBase->Flags = Class->Flags;
	NtGdiDiInsert( Window, ( PDI_BASE )WindowBase );
}

VOID
NtGdiDiWindowBaseDrawProc(
	__in PKWINDOW Window,
	__in PDI_WINDOW_BASE WindowBase
)
{

	ULONG32 Width = Window->Rect.right - Window->Rect.left;
	ULONG32 Height = Window->Rect.bottom - Window->Rect.top;

	for ( ULONG32 i = 0; i < ( Width * Height ); i++ ) {

		Window->PrimaryBuffer[ i ] = WindowBase->Fill;
	}

	if ( Window == g_RootWindow ) {

		return;
	}

	for ( ULONG32 i = 0; i < ( Width * WND_TITLE_BAR_H ); i++ ) {

		Window->PrimaryBuffer[ i ] = 0xFFFFFFFF;
	}

	ULONG32 Cross = 0xFF999999;
	if ( g_Focus->Parent == Window ) {

		Cross = 0xFF050708;
	}

	Window->PrimaryBuffer[ Width * ( 17 - 4 ) - 20 - 1 ] = Cross;
	Window->PrimaryBuffer[ Width * ( 17 - 3 ) - 19 - 1 ] = Cross;
	Window->PrimaryBuffer[ Width * ( 17 - 2 ) - 18 - 1 ] = Cross;
	Window->PrimaryBuffer[ Width * ( 17 - 1 ) - 17 - 1 ] = Cross;
	Window->PrimaryBuffer[ Width * ( 17 + 0 ) - 16 - 1 ] = Cross;
	Window->PrimaryBuffer[ Width * ( 17 + 1 ) - 15 - 1 ] = Cross;
	Window->PrimaryBuffer[ Width * ( 17 + 2 ) - 14 - 1 ] = Cross;
	Window->PrimaryBuffer[ Width * ( 17 + 3 ) - 13 - 1 ] = Cross;
	Window->PrimaryBuffer[ Width * ( 17 + 4 ) - 12 - 1 ] = Cross;

	Window->PrimaryBuffer[ Width * ( 17 - 4 ) - 12 - 1 ] = Cross;
	Window->PrimaryBuffer[ Width * ( 17 - 3 ) - 13 - 1 ] = Cross;
	Window->PrimaryBuffer[ Width * ( 17 - 2 ) - 14 - 1 ] = Cross;
	Window->PrimaryBuffer[ Width * ( 17 - 1 ) - 15 - 1 ] = Cross;
	Window->PrimaryBuffer[ Width * ( 17 + 0 ) - 16 - 1 ] = Cross;
	Window->PrimaryBuffer[ Width * ( 17 + 1 ) - 17 - 1 ] = Cross;
	Window->PrimaryBuffer[ Width * ( 17 + 2 ) - 18 - 1 ] = Cross;
	Window->PrimaryBuffer[ Width * ( 17 + 3 ) - 19 - 1 ] = Cross;
	Window->PrimaryBuffer[ Width * ( 17 + 4 ) - 20 - 1 ] = Cross;

	g_DefaultFont->Render( g_DefaultFont, Window->Name.Buffer, WND_TITLE_INDENT_X, WND_TITLE_INDENT_Y, Cross, Window->PrimaryBuffer, &Window->Rect );

	ULONG32 Border = 0x80000000;

	if ( g_Focus->Parent == Window ) {

		Border = 0xFF000000;
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

}
