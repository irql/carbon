


#pragma once

#include "svga_reg.h"

typedef struct _SVGA_DEVICE {

	PPCI_DEVICE PciDevice;
	USHORT      IoBase;

	ULONG32*    FifoBase;
	ULONG32*    FramebufferBase;

	ULONG32     FifoLength;
	ULONG32     FramebufferLength;

	ULONG32	    VideoMemoryLength;

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

EXTERN SVGA_DEVICE g_SVGA;

VOID
SvDriverInitialize(

);

VOID
SvSetMode(
	__in ULONG32 Width,
	__in ULONG32 Height,
	__in ULONG32 Bpp
);

ULONG32
SvReadRegister(
	__in ULONG32 Register
);

VOID
SvWriteRegister(
	__in ULONG32 Register,
	__in ULONG32 Value
);

BOOLEAN
SvFifoRegisterValid(
	__in ULONG32 Register
);

BOOLEAN
SvFifoCapabilityValid(
	__in ULONG32 Capability
);

PVOID
SvFifoReserve(
	__in ULONG32 ByteCount
);

VOID
SvFifoCommit(
	__in ULONG32 ByteCount
);

VOID
SvFifoCommitAll(

);

PVOID
SvFifoReserveCmd(
	__in ULONG32 Type,
	__in ULONG32 ByteCount
);

VOID
SvFifoFull(

);

VOID
SvUpdate(
	__in ULONG32 x,
	__in ULONG32 y,
	__in ULONG32 Width,
	__in ULONG32 Height
);

VOID
SvBeginDefineCursor(
	__in SVGAFifoCmdDefineCursor* CursorInfo,
	__out PVOID* AndMask,
	__out PVOID* XorMask
);

VOID
SvBeginDefineAlphaCursor(
	__in SVGAFifoCmdDefineAlphaCursor* CursorInfo,
	__out PVOID* Cursor
);

VOID
SvMoveCursor(
	__in ULONG32 Visible,
	__in ULONG32 x,
	__in ULONG32 y
);

ULONG32
SvFenceInsert(

);

BOOLEAN
SvFencePassed(
	__in ULONG32 Fence
);

VOID
SvFenceSync(
	__in ULONG32 Fence
);