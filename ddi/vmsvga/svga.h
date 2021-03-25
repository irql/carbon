


#pragma once

#include "svga_reg.h"

typedef struct _SVGA_DEVICE {

    PDEVICE_OBJECT PciDevice;
    USHORT      IoBase;

    ULONG32*    FifoBase;
    ULONG32*    FramebufferBase;

    ULONG32     FifoLength;
    ULONG32     FramebufferLength;

    ULONG32     VideoMemoryLength;

    ULONG32     DeviceVersionId;
    ULONG32     DeviceCapabilities;

    ULONG32     Width;
    ULONG32     Height;
    ULONG32     Bpp;
    ULONG32     Pitch;

    struct {
        ULONG32 ReservedSize;
        BOOLEAN UsingBounceBuffer;
        UCHAR   BounceBuffer[ 1024 * 1024 ];
        ULONG32 NextFence;

    } FifoReserve;

} SVGA_DEVICE, *PSVGA_DEVICE;

VOID
DdDriverInitialize(
    _In_ PDRIVER_OBJECT DriverObject
);

VOID
DdSetMode(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      Width,
    _In_ ULONG32      Height,
    _In_ ULONG32      Bpp
);

ULONG32
DdReadRegister(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      Register
);

VOID
DdWriteRegister(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      Register,
    _In_ ULONG32      Value
);

BOOLEAN
DdFifoRegisterValid(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      Register
);

BOOLEAN
DdFifoCapabilityValid(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      Capability
);

PVOID
DdFifoReserve(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      ByteCount
);

VOID
DdFifoCommit(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      ByteCount
);

VOID
DdFifoCommitAll(
    _In_ PSVGA_DEVICE Svga
);

PVOID
DdFifoReserveCmd(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      Type,
    _In_ ULONG32      ByteCount
);

VOID
DdFifoFull(

);

VOID
DdUpdate(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      x,
    _In_ ULONG32      y,
    _In_ ULONG32      Width,
    _In_ ULONG32      Height
);

#if 0
VOID
SvBeginDefineCursor(
    _In_ SVGAFifoCmdDefineCursor* CursorInfo,
    _Out_ PVOID* AndMask,
    _Out_ PVOID* XorMask
);

VOID
SvBeginDefineAlphaCursor(
    _In_ SVGAFifoCmdDefineAlphaCursor* CursorInfo,
    _Out_ PVOID* Cursor
);

VOID
SvMoveCursor(
    _In_ ULONG32 Visible,
    _In_ ULONG32 x,
    _In_ ULONG32 y
);
#endif

ULONG32
DdFenceInsert(
    _In_ PSVGA_DEVICE Svga
);

BOOLEAN
DdFencePassed(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      Fence
);

VOID
DdFenceSync(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      Fence
);
