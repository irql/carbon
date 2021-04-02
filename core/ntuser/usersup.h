


#pragma once

//
// TODO: fix up this header and others!!!
//

#ifndef USER_INTERNAL
#define NTUSERAPI DLLEXPORT
#elif !defined( NTUSER_INTERNAL ) 
#define NTUSERAPI DLLIMPORT
#else
#define NTUSERAPI
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

NTUSERAPI
VOID
NtSendMessage(
    _In_ HANDLE  WindowHandle,
    _In_ ULONG32 MessageId,
    _In_ ULONG64 Param1,
    _In_ ULONG64 Param2
);

NTUSERAPI
VOID
NtSendParentMessage(
    _In_ HANDLE  WindowHandle,
    _In_ ULONG32 MessageId,
    _In_ ULONG64 Param1,
    _In_ ULONG64 Param2
);

NTUSERAPI
BOOLEAN
NtReceiveMessage(
    _In_  HANDLE         WindowHandle,
    _Out_ PKUSER_MESSAGE Message
);

NTUSERAPI
VOID
NtWaitMessage(
    _In_ HANDLE WindowHandle
);

NTUSERAPI
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

NTUSERAPI
VOID
NtGetWindowInfo(
    _In_  HANDLE    WindowHandle,
    _Out_ PWND_INFO WindowInfo
);

NTUSERAPI
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

NTUSERAPI
NTSTATUS
NtRegisterClass(
    _In_ PWND_CLASS Class
);

NTUSERAPI
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

NTUSERAPI
NTSTATUS
NtDefaultWindowProc(
    _In_ HANDLE  WindowHandle,
    _In_ ULONG32 MessageId,
    _In_ ULONG64 Param1,
    _In_ ULONG64 Param2
);

NTUSERAPI
NTSTATUS
NtGetWindowProc(
    _In_  HANDLE    WindowHandle,
    _Out_ WND_PROC* WndProc
);

NTUSERAPI
VOID
NtBeginPaint(
    _Out_ PHANDLE ContextHandle,
    _In_  HANDLE  WindowHandle
);

NTUSERAPI
VOID
NtEndPaint(
    _In_ HANDLE WindowHandle
);

NTUSERAPI
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

NTUSERAPI
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

NTUSERAPI
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

NTUSERAPI
ULONG64
NtGetTickCount(

);

NTUSERAPI
VOID
NtClearDC(
    _In_ HANDLE  ContextHandle,
    _In_ ULONG32 x,
    _In_ ULONG32 y,
    _In_ ULONG32 w,
    _In_ ULONG32 h,
    _In_ ULONG32 Color
);

NTUSERAPI
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

typedef struct _LV_ITEM {
    ULONG32          Id;
    PWSTR            Name;

    struct _LV_ITEM* Link;

} LV_ITEM, *PLV_ITEM;

#define LV_INSERTITEM   0xF0
#define LV_GETSELECTED  0xF1
#define LV_SETSELECTED  0xF2
#define LV_GETITEMCOUNT 0xF3
#define LV_GETITEM      0xF4
#define LV_REMOVEITEM   0xF5

#define LV_SELECTED     0xE0
#define LV_PRESSED      0xE1

#define ED_ENTER        0xF0

FORCEINLINE
ULONG64
CLAMP(
    _In_ ULONG64 Val,
    _In_ ULONG64 Min,
    _In_ ULONG64 Max
)
{
    return Val < Min ? Min : ( Val > Max ? Max : Val );
}
