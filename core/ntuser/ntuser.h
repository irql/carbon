


#pragma once

typedef struct _KWND {

    HANDLE         MessageEvent; // change to KEVENT
    PKUSER_MESSAGE MessageQueue;
    KSPIN_LOCK     MessageQueueLock;

    WND_INFO       WindowInfo;

    //
    // This interface is severly under developed, these
    // should not be direct buffers, but instead a handle of
    // sorts for the graphics drivers.
    //
    // Implement something like device contexts, user mode
    // programs should not be able to decide their client area 
    // completely? or at least it should be some-what managed
    // by graphics drivers
    //

    PDC            BackContext;
    PDC            FrontContext;
    BOOLEAN        ContextUpdate;
    BOOLEAN        PaintBegan;

    PWND_CLASS     WindowClass;

    KSPIN_LOCK     LinkLock;
    PKWND          Next;
    PKWND          Child;
    PKWND          Parent;

    //
    // WM stuff
    //

    PVOID Font;
    PVOID Uptr;
    LONG64 ScrollH;
    LONG64 ScrollW;

} KWND, *PKWND;

typedef struct _KUSER_MESSAGE {

    ULONG32        MessageId;
    ULONG64        Param1;
    ULONG64        Param2;

    PKWND          WindowObject;
    PKUSER_MESSAGE MessageQueue;

} KUSER_MESSAGE, *PKUSER_MESSAGE;

EXTERN PLIST_ENTRY    WindowLinks;
EXTERN KSPIN_LOCK     WindowLock;

EXTERN POBJECT_TYPE   NtWindowObject;

#if 0
EXTERN PKUSER_MESSAGE SystemMessageQueue;
EXTERN KSPIN_LOCK     SystemMessageQueueLock;
EXTERN HANDLE         SystemMessageEvent;
#endif
EXTERN IO_QUEUE       SystemMessageQueue;
EXTERN KEVENT         SystemMessageEvent;

PKUSER_MESSAGE
NtAllocateUserMessage(

);

VOID
NtFreeUserMessage(
    _In_ PKUSER_MESSAGE Message
);

typedef struct _WND_CLASS {

    WND_PROC   WndProc;
    WCHAR      ClassName[ 64 ];
    ULONG32    Flags;
    ULONG32    Border;
    ULONG32    Fill;

    LIST_ENTRY ClassLinks;
    WND_PROC   DefWndProc;

} WND_CLASS, *PWND_CLASS;

NTSTATUS
NtClassWindowBaseProc(
    _In_ PKWND   Window,
    _In_ ULONG32 MessageId,
    _In_ ULONG64 Param1,
    _In_ ULONG64 Param2
);

VOID
NtInitializeUserWindows(

);

EXTERN VOLATILE PKWND RootWindow;
EXTERN VOLATILE PKWND FocusWindow;
EXTERN POBJECT_TYPE   NtDeviceContext;

NTSTATUS
NtFindClassByName(
    _Out_ PWND_CLASS* WindowClass,
    _In_  PWSTR       ClassName
);

PKWND
NtWindowFromPoint(
    _In_ ULONG32 x,
    _In_ ULONG32 y
);

NTSTATUS
NtDdiCreateDC(
    _Out_ PDC*  DeviceContext,
    _In_  PRECT Rect
);

VOID
NtDdiBlt(
    _In_ PDC     SourceContext,
    _In_ ULONG32 srcx,
    _In_ ULONG32 srcy,
    _In_ ULONG32 srcw,
    _In_ ULONG32 srch,
    _In_ PDC     DestinationContext,
    _In_ ULONG32 dstx,
    _In_ ULONG32 dsty
);

VOID
NtDdiBltBits(
    _In_ PVOID   srcbits,
    _In_ ULONG32 srcx,
    _In_ ULONG32 srcy,
    _In_ ULONG32 srcw,
    _In_ ULONG32 srch,
    _In_ PDC     context,
    _In_ ULONG32 dstx,
    _In_ ULONG32 dsty
);

VOID
NtDdiBeginPaint(
    _Out_ PDC*  Context,
    _In_  PKWND Window
);

VOID
NtDdiEndPaint(
    _In_ PKWND Window
);

VOID
NtDdiClearDC(
    _In_ PDC     Context,
    _In_ ULONG32 x,
    _In_ ULONG32 y,
    _In_ ULONG32 w,
    _In_ ULONG32 h,
    _In_ ULONG32 Color
);

VOID
NtBroadcastDirectMessage(
    _In_ ULONG32 MessageId,
    _In_ ULONG64 Param1,
    _In_ ULONG64 Param2
);

EXTERN PDC    NtScreenDC;
EXTERN PDC    Composed;
EXTERN HANDLE UpdateEvent;

VOID
NtDrawCursor(

);

#define FONT_RASTER     0x01000000
#define FONT_TRUE_TYPE  0x02000000
#define FONT_OPEN_TYPE  0x04000000
#define FONT_VECTOR     0x08000000

typedef struct _KFONT *PKFONT;

typedef VOID( *FONT_RENDER )(
    _In_  PKFONT,
    _In_  PDC,
    _In_  PWSTR,
    _In_  PRECT,
    _In_  ULONG
    );
typedef VOID( *FONT_EXTENT )(
    _In_  PKFONT,
    _In_  PDC,
    _In_  PWSTR,
    _Out_ PRECT
    );

typedef struct _KFONT_ENGINE {
    FONT_RENDER Render;
    FONT_EXTENT Extent;

} KFONT_ENGINE, *PKFONT_ENGINE;

typedef struct _KFONT {
    //
    // Completely deprecated, and only used 
    // for very primitive dogshit, check user.dll
    //

    HANDLE        FileHandle;
    HANDLE        SectionHandle;
    PVOID         FileMapping;
    ULONG         FontFlags;
    PKFONT_ENGINE Engine;
    ULONG         FontWidth;
    ULONG         FontHeight;

} KFONT, *PKFONT;

EXTERN POBJECT_TYPE NtFontObject;
EXTERN PKFONT       NtSystemFont;

VOID
NtInitializeUserFonts(

);

NTSTATUS
NtCreateFont(
    _Out_ PHANDLE FontHandle,
    _In_  PWSTR   FontFace,
    _In_  ULONG   FontWidth,
    _In_  ULONG   FontHeight,
    _In_  ULONG   FontFlags
);

NTSTATUS
NtRenderFont(
    _In_ HANDLE FontHandle,
    _In_ HANDLE ContextHandle,
    _In_ PWSTR  String,
    _In_ PRECT  Clip,
    _In_ ULONG  Color
);

VOID
NtDdiSetPixel(
    _In_ PDC     Context,
    _In_ ULONG32 x,
    _In_ ULONG32 y,
    _In_ ULONG32 Color
);

VOID
NtRenderRasterFont(
    _In_ PKFONT FontObject,
    _In_ PDC    ContextObject,
    _In_ PWSTR  String,
    _In_ PRECT  Clip,
    _In_ ULONG  Color
);

BOOLEAN
NtClassStaticBaseProc(
    _In_ PKWND   Window,
    _In_ ULONG32 MessageId,
    _In_ ULONG64 Param1,
    _In_ ULONG64 Param2
);

BOOLEAN
NtClassButtonBaseProc(
    _In_ PKWND   Window,
    _In_ ULONG32 MessageId,
    _In_ ULONG64 Param1,
    _In_ ULONG64 Param2
);

VOID
NtExtentRasterFont(
    _In_  PKFONT FontObject,
    _In_  PDC    ContextObject,
    _In_  PWSTR  String,
    _Out_ PRECT  Extent
);

VOID
NtInitializeSystemMessages(

);

VOID
NtRemoveWindow(
    _In_ PKWND Remove
);

VOID
NtInsertWindow(
    _In_ PKWND Insert
);
