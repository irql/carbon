


#include "driver.h"

BOOLEAN g_TypingCursor = FALSE;
RECT g_TypingCursorPosition = { 0 };

VOID
NtGdiTypingCursorRender(

)
{

	if ( !g_TypingCursor ) {

		return;
	}

	for ( ULONG32 i = 0; i < ( g_TypingCursorPosition.bottom - g_TypingCursorPosition.top ); i++ ) {

		if ( g_TypingCursorPosition.left >= g_Basic.Width ) {

			continue;
		}

		if ( ( g_TypingCursorPosition.top + i ) >= g_Basic.Height ) {

			continue;
		}

		g_Basic.Framebuffer[ ( g_TypingCursorPosition.top + i ) * g_Basic.Width + g_TypingCursorPosition.left ] =
			( ~g_Basic.Framebuffer[ ( g_TypingCursorPosition.top + i ) * g_Basic.Width + g_TypingCursorPosition.left ] ) | 0xFF000000;

		//NtGdiPutPixel2( g_Basic.Framebuffer, g_TypingCursorPosition.left, g_TypingCursorPosition.top + i, 0xFF000000, g_Basic.Width, g_Basic.Height );
	}


}

