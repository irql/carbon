


#pragma once

#include <carbsup.h>
#include "bmp.h"

#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))

//KDCOM.DLL IMPORT.
DECLSPEC( DLLIMPORT )
VOID
KdCmdMessage(
	__in PWCHAR Message,
	__in ...
);

typedef struct _KBASIC_INFO {
	ULONG32  Width;
	ULONG32  Height;

	ULONG32* Framebuffer;
	ULONG32* Doublebuffer;


} KBASIC_INFO, *PKBASIC_INFO;

EXTERN KBASIC_INFO g_Basic;

void*
memcpy_SSE2_4KA(
	__in void* m1,
	__in void* m2,
	__in __int64 c
);

void*
memset_SSE2_4KA(
	__in void* m1,
	__in __int64 v,
	__in __int64 c
);

ULONG32
FORCEINLINE
NtGdiAlphaBlend(
	__in ULONG32 Src,
	__in ULONG32 Dst
)
{
	//https://www.daniweb.com/programming/software-development/code/216791/alpha-blend-algorithm

	UCHAR Alpha = ( UCHAR )( Src >> 24 );

	if ( Alpha == 255 )
		return Src;

	if ( Alpha == 0 )
		return Dst;

	ULONG32 RedBlue = ( ( ( Src & 0x00ff00ff ) * Alpha ) + ( ( Dst & 0x00ff00ff ) * ( 0xff - Alpha ) ) ) & 0xff00ff00;
	ULONG32 Green = ( ( ( Src & 0x0000ff00 ) * Alpha ) + ( ( Dst & 0x0000ff00 ) * ( 0xff - Alpha ) ) ) & 0x00ff0000;

	return ( Src & 0xff000000 ) | ( ( RedBlue | Green ) >> 8 );
}

VOID
FORCEINLINE
NtGdiPutPixel(
	__in ULONG32* Buffer,
	__in ULONG32 x,
	__in ULONG32 y,
	__in ULONG32 Colour
)
{

	if ( x >= g_Basic.Width )
		return;
	if ( y >= g_Basic.Height )
		return;

	Buffer[ y * g_Basic.Width + x ] = NtGdiAlphaBlend( Colour, Buffer[ y * g_Basic.Width + x ] );
}

VOID
FORCEINLINE
NtGdiPutPixel2(
	__in ULONG32* Buffer,
	__in ULONG32 x,
	__in ULONG32 y,
	__in ULONG32 Colour,
	__in ULONG32 Width,
	__in ULONG32 Height
)
{

	if ( x >= Width )
		return;
	if ( y >= Height )
		return;

	Buffer[ y * Width + x ] = NtGdiAlphaBlend( Colour, Buffer[ y * Height + x ] );
}

typedef struct _POINT {
	ULONG32 x;
	ULONG32 y;
} POINT, *PPOINT;

typedef struct _SIGNED_POINT {
	LONG32 x;
	LONG32 y;
} SIGNED_POINT, *PSIGNED_POINT;

typedef struct _RECT {
	ULONG32 left;
	ULONG32 top;
	ULONG32 right;
	ULONG32 bottom;
} RECT, *PRECT;

VOID
NtSvRenderLoop(

);

VOID
NtGdiRenderLoop(

);

EXTERN UNICODE_STRING g_DefaultCursorDirectoryFile;
EXTERN POINT g_NtCursorPosition;

VOID
NtMouseInstall(

);

VOID
NtKeyboardInstall(

);

#define KEY_FLAG_CTRL	0x80
#define KEY_FLAG_SHIFT	0x40
#define KEY_FLAG_ALT	0x20

typedef enum _STATE {
	KeyStatePress,
	KeyStateRelease,
	KeyStateMax
} STATE;

typedef UCHAR KEY_STATE;

typedef struct _KEYBOARD_PACKET {
	//UCS-2 Encoded.
	WCHAR Character;
	KEY_STATE State;
	UCHAR Flags;
} KEYBOARD_PACKET, *PKEYBOARD_PACKET;

typedef struct _MOUSE_PACKET {
	UCHAR Button;
	KEY_STATE State;
	POINT Position;
	UCHAR Flags;//something like +/- mouse wheel movement?
} MOUSE_PACKET, *PMOUSE_PACKET;


typedef struct _KCURSOR {
	ULONG32  Width;
	ULONG32  Height;
	ULONG32* Buffer;

} KCURSOR, *PKCURSOR;

EXTERN KCURSOR g_Cursor;

NTSTATUS
NtInitializeCursor(

);

VOID
NtGdiMouseHandleClick(
	__in PMOUSE_PACKET MousePacket
);

VOID
NtGdiMouseHandleMove(
	__in POINT OldPosition,
	__in POINT NewPosition
);

#include "font.h"

#include "wnd.h"

#define CONSOLE_DEFAULT_HEIGHT 450
#define CONSOLE_DEFAULT_WIDTH  600

#define CON_FLAG_READING 0x80

typedef struct _KCONSOLE {

	PKWINDOW Console;

	ULONG32 Flags;

} KCONSOLE, *PKCONSOLE;

NTSTATUS
NtGdiConsoleInitializeSubsystem(

);

NTSTATUS
NtGdiCreateConsole(
	__out PHANDLE ConsoleHandle,
	__in PUNICODE_STRING Name,
	__in ULONG32 Flags,
	__in ULONG32 x,
	__in ULONG32 y
);

NTSTATUS
NtGdiReadConsole(
	__in HANDLE ConsoleHandle,
	__in PWCHAR Buffer,
	__in ULONG32 Length
);

NTSTATUS
NtGdiWriteConsole(
	__in HANDLE ConsoleHandle,
	__in PWCHAR Buffer,
	__in ULONG32 Length
);

PKCONSOLE
NtGdiReferenceConsoleFromWindow(
	__in PKWINDOW Window
);

#define EDITBOX_REJECT_INPUT (0x00000001L)
#define EDITBOX_NO_FOCUS_OUTLINE (0x00000002L)

#define WND_PRIMARY_COLOUR (0xFF0078D7L)


EXTERN BOOLEAN g_CursorVisible;
EXTERN BOOLEAN g_TypingCursor;
EXTERN RECT g_TypingCursorPosition;

VOID
NtGdiTypingCursorRender(

);
