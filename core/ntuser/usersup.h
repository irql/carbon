


#pragma once

//
// TODO: fix up this header and others!!!
//

#ifndef USER_INTERNAL
#define NTUSRAPI DLLEXPORT
#elif !defined( NTUSER_INTERNAL ) 
#define NTUSRAPI DLLIMPORT
#else
#define NTUSRAPI
#endif

#ifdef NTUSER_INTERNAL
#define NTAPI DLLEXPORT
#else
#define NTAPI DLLIMPORT
#endif

#define USER_TAG 'resU'

typedef struct _RECT {
    LONG32 Left;
    LONG32 Top;
    LONG32 Right;
    LONG32 Bottom;
} RECT, *PRECT;

typedef struct _DC {
    RECT           ClientArea;

    // add to cleanup v
    PDEVICE_OBJECT DeviceObject;
    PVOID          DeviceSpecific;
} DC, *PDC;

typedef enum _KEY_STATE {
    KeyStatePress,
    KeyStateRelease,
} KEY_STATE, *PKEY_STATE;

typedef struct _KWND *PKWND;
typedef struct _KUSER_MESSAGE *PKUSER_MESSAGE;

#ifndef NTUSER_INTERNAL
typedef struct _KUSER_MESSAGE {

    ULONG32 MessageId;
    ULONG64 Param1;
    ULONG64 Param2;

} KUSER_MESSAGE, *PKUSER_MESSAGE;
#endif

VOID
NtReceiveSystemMessage(
    _In_ PKUSER_MESSAGE Buffer
);

NTAPI
VOID
NtSendSystemMessage(
    _In_ ULONG32 MessageId,
    _In_ ULONG64 Param1,
    _In_ ULONG64 Param2
);

#define WM_KEYDOWN          0x01
#define WM_KEYUP            0x02
#define WM_LMOUSEDOWN       0x03
#define WM_LMOUSEUP         0x04
#define WM_RMOUSEDOWN       0x05
#define WM_RMOUSEUP         0x06
#define WM_PAINT            0x07
#define WM_ACTIVATE         0x08
#define WM_FOCUS            0x09
#define WM_SETTEXT          0x0A
#define WM_GETTEXT          0x0B
#define WM_GETTEXTLENGTH    0x0C
#define WM_SETFONT          0x0D
#define WM_GETFONT          0x0E

#define WM_SETUPTR          0x0F
#define WM_GETUPTR          0x10

#define WM_GETSCROLLH       0x11
#define WM_GETSCROLLW       0x12
#define WM_SETSCROLLH       0x13
#define WM_SETSCROLLW       0x14
#define WM_VSCROLL          0x15
#define WM_HSCROLL          0x16

#define WM_COMMAND          0x17

#define WM_SETMENU          0x18
#define WM_GETMENU          0x19

#define VK_ENTER    '\n'
#define VK_BACK     '\b'
#define VK_TAB      '\t'
#define VK_CTRL     0xF0
#define VK_ALT      0xF1
#define VK_UP       0xF2
#define VK_DOWN     0xF3
#define VK_LEFT     0xF4
#define VK_RIGHT    0xF5
#define VK_PGUP     0xF6
#define VK_PGDOWN   0xF7
#define VK_HOME     0xF8
#define VK_DEL      0xF9
#define VK_END      0xFA
#define VK_INS      0xFB

#define VK_F1       0xE0
#define VK_F2       0xE1
#define VK_F3       0xE2
#define VK_F4       0xE3
#define VK_F5       0xE4
#define VK_F6       0xE5
#define VK_F7       0xE6
#define VK_F8       0xE7
#define VK_F9       0xE8
#define VK_F10      0xE9
#define VK_F11      0xEA
#define VK_F12      0xEB

NTUSRAPI
VOID
NtSendMessage(
    _In_ HANDLE  WindowHandle,
    _In_ ULONG32 MessageId,
    _In_ ULONG64 Param1,
    _In_ ULONG64 Param2
);

NTUSRAPI
VOID
NtSendParentMessage(
    _In_ HANDLE  WindowHandle,
    _In_ ULONG32 MessageId,
    _In_ ULONG64 Param1,
    _In_ ULONG64 Param2
);

NTUSRAPI
BOOLEAN
NtReceiveMessage(
    _In_  HANDLE         WindowHandle,
    _Out_ PKUSER_MESSAGE Message
);

NTUSRAPI
VOID
NtWaitMessage(
    _In_ HANDLE WindowHandle
);

NTUSRAPI
NTSTATUS
NtCreateWindow(
    _Out_    PHANDLE WindowHandle,
    _In_opt_ HANDLE  ParentHandle,
    _In_     PWSTR   Name,
    _In_     PWSTR   Class,
    _In_     ULONG32 x,
    _In_     ULONG32 y,
    _In_     ULONG32 w,
    _In_     ULONG32 h,
    _In_     ULONG32 MenuId
);

typedef struct _WND_INFO {
    ULONG32    MenuId;
    WCHAR      Name[ 512 ];
    RECT       Rect;

} WND_INFO, *PWND_INFO;

NTUSRAPI
VOID
NtGetWindowInfo(
    _In_  HANDLE    WindowHandle,
    _Out_ PWND_INFO WindowInfo
);

NTUSRAPI
VOID
NtSetWindowInfo(
    _In_ HANDLE    WindowHandle,
    _In_ PWND_INFO WindowInfo
);

typedef struct _WND_CLASS *PWND_CLASS;

typedef NTSTATUS( *WND_PROC )(
    _In_ HANDLE  Window,
    _In_ ULONG32 MessageId,
    _In_ ULONG64 Param1,
    _In_ ULONG64 Param2
    );

#ifndef NTUSER_INTERNAL
typedef struct _WND_CLASS {

    WND_PROC   WndProc;
    WCHAR      ClassName[ 64 ];
    ULONG32    Flags;
    ULONG32    Border;
    ULONG32    Fill;

} WND_CLASS, *PWND_CLASS;
#endif

NTUSRAPI
NTSTATUS
NtRegisterClass(
    _In_ PWND_CLASS Class
);

NTUSRAPI
VOID
NtUnregisterClass(
    _In_ PWND_CLASS Class
);

VOID
NtSendDirectMessage(
    _In_ PKWND   WindowObject,
    _In_ ULONG32 MessageId,
    _In_ ULONG64 Param1,
    _In_ ULONG64 Param2
);

NTUSRAPI
NTSTATUS
NtDefaultWindowProc(
    _In_ HANDLE  WindowHandle,
    _In_ ULONG32 MessageId,
    _In_ ULONG64 Param1,
    _In_ ULONG64 Param2
);

NTUSRAPI
NTSTATUS
NtGetWindowProc(
    _In_  HANDLE    WindowHandle,
    _Out_ WND_PROC* WndProc
);

NTUSRAPI
VOID
NtBeginPaint(
    _Out_ PHANDLE ContextHandle,
    _In_  HANDLE  WindowHandle
);

NTUSRAPI
VOID
NtEndPaint(
    _In_ HANDLE WindowHandle
);

NTUSRAPI
VOID
NtBlt(
    _In_ HANDLE  SourceContext,
    _In_ ULONG32 srcx,
    _In_ ULONG32 srcy,
    _In_ ULONG32 srcw,
    _In_ ULONG32 srch,
    _In_ HANDLE  DestinationContext,
    _In_ ULONG32 dstx,
    _In_ ULONG32 dsty
);

NTUSRAPI
VOID
NtBltBits(
    _In_ PVOID   srcbits,
    _In_ ULONG32 srcx,
    _In_ ULONG32 srcy,
    _In_ ULONG32 srcw,
    _In_ ULONG32 srch,
    _In_ HANDLE  context,
    _In_ ULONG32 dstx,
    _In_ ULONG32 dsty
);

NTUSRAPI
NTSTATUS
NtGetDC(
    _Out_ PHANDLE ContextHandle,
    _In_  HANDLE  WindowHandle
);

NTAPI
VOID
NtSetCursorPosition(
    _In_ LONG32 posx,
    _In_ LONG32 posy
);

NTAPI
VOID
NtGetCursorPosition(
    _Out_ ULONG32* posx,
    _Out_ ULONG32* posy
);

NTUSRAPI
ULONG64
NtGetTickCount(

);

NTUSRAPI
VOID
NtClearDC(
    _In_ HANDLE  ContextHandle,
    _In_ ULONG32 x,
    _In_ ULONG32 y,
    _In_ ULONG32 w,
    _In_ ULONG32 h,
    _In_ ULONG32 Color
);

NTUSRAPI
VOID
NtSetPixel(
    _In_ HANDLE  ContextHandle,
    _In_ ULONG32 x,
    _In_ ULONG32 y,
    _In_ ULONG32 Color
);

NTAPI
VOID
NtGetMode(
    _In_ ULONG32* Width,
    _In_ ULONG32* Height,
    _In_ ULONG32* BitDepth
);

NTAPI
NTSTATUS
NtSetMode(
    _In_ ULONG32 Width,
    _In_ ULONG32 Height,
    _In_ ULONG32 BitDepth
);

NTUSRAPI
NTSTATUS
NtGetWindowByName(
    _Out_ PHANDLE WindowHandle,
    _In_  PWSTR   WindowName,
    _In_  PWSTR   ClassName
);

NTUSRAPI
NTSTATUS
NtSetParent(
    _In_ HANDLE ParentHandle,
    _In_ HANDLE WindowHandle
);
