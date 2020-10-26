


#include "driver.h"
#include "wnd.h"

#include "svga.h"

VOID
NtSvRenderLoop(

)
{
	STATIC ULONG32 Fence = 0;

	while ( 1 ) {

		SvFenceSync( Fence );
		Fence = SvFenceInsert( );

		KeAcquireSpinLock( &g_WindowListLock );
		PKWINDOW Window = g_RootWindow;

		while ( Window != NULL ) {

			NtGdiWindowComposite( Window );

			ULONG32 Width = Window->Rect.right - Window->Rect.left;
			ULONG32 Height = Window->Rect.bottom - Window->Rect.top;

			POINT CurrentPixel = { 0 };
			for ( ULONG32 i = 0; i < ( Width * Height ); i++ ) {

				NtGdiPutPixel( g_Basic.Doublebuffer, Window->Rect.left + CurrentPixel.x, Window->Rect.top + CurrentPixel.y, Window->PrimaryBuffer[ i ] );

				CurrentPixel.x++;

				if ( CurrentPixel.x >= Width ) {

					CurrentPixel.y++;
					CurrentPixel.x = 0;
				}
			}

			Window = Window->Next;
		}

		KeReleaseSpinLock( &g_WindowListLock );

		_memcpy( g_Basic.Framebuffer, g_Basic.Doublebuffer, g_Basic.Height * g_Basic.Width * 4 );

		NtGdiTypingCursorRender( );

		SvUpdate( 0, 0, g_Basic.Width, g_Basic.Height );

		//KeDelayExecutionThread( 14 );
	}
}

VOID
NtGdiRenderLoop(

)
{
#if 0
	STATIC ULONG32 Fence = 0;

	while ( 1 ) {

		SvFenceSync( Fence );
		Fence = SvFenceInsert( );

		KeAcquireSpinLock( &WindowList.Lock );
		PLIST_ENTRY Flink = WindowList.List;

		do {
			PKWINDOW Window = CONTAINING_RECORD( Flink, KWINDOW, WindowLinks );

			NtGdiWindowComposite( Window );

			ULONG32 Width = Window->Rect.right - Window->Rect.left;
			ULONG32 Height = Window->Rect.bottom - Window->Rect.top;

			POINT CurrentPixel = { 0 };
			for ( ULONG32 i = 0; i < ( Width * Height ); i++ ) {

				NtGdiPutPixel( g_Basic.Doublebuffer, Window->Rect.left + CurrentPixel.x, Window->Rect.top + CurrentPixel.y, Window->PrimaryBuffer[ i ] );

				CurrentPixel.x++;

				if ( CurrentPixel.x >= Width ) {

					CurrentPixel.y++;
					CurrentPixel.x = 0;
				}
			}

			Flink = Flink->Flink;
		} while ( Flink != WindowList.List );

		KeReleaseSpinLock( &WindowList.Lock );

		_memcpy( g_Basic.Framebuffer, g_Basic.Doublebuffer, g_Basic.Height * g_Basic.Width * 4 );

#if 0
		POINT CurrentPixel = { 0, CursorBitmapHeader->Height };
		for ( ULONG32 i = 0; i <= CursorBitmapHeader->Height * CursorBitmapHeader->Width; i++ ) {

			NtGdiPutPixel( Doublebuffer, NtCursorPosition.x + CurrentPixel.x, NtCursorPosition.y + CurrentPixel.y, CursorBitmapPixels[ i ] );

			CurrentPixel.x++;

			if ( ( CurrentPixel.x ) >= CursorBitmapHeader->Width ) {

				CurrentPixel.y--;
				CurrentPixel.x = 0;
			}
		}
#endif

		SvUpdate( 0, 0, g_Basic.Width, g_Basic.Height );

		//KeDelayExecutionThread( 16 );
	}
#endif

}

