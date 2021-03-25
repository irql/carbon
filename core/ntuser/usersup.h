


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

#define WM_KEYDOWN      0x01
#define WM_KEYUP        0x02
#define WM_LMOUSEDOWN   0x03
#define WM_LMOUSEUP     0x04
#define WM_RMOUSEDOWN   0x05
#define WM_RMOUSEUP     0x06
#define WM_PAINT        0x07
#define WM_ACTIVATE     0x08
#define WM_FOCUS        0x09

NTUSERAPI
VOID
NtSendMessage(
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

typedef BOOLEAN( *WND_PROC )(
    _In_ PKWND   Window,
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
BOOLEAN
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
