

#include "driver.h"

#include "svga.h"
#include "svga3d.h"

//
//	a lot of code is ripped from the vmware reference driver
//	https://github.com/prepare/vmware-svga/blob/master/lib/refdriver/
//

SVGA_DEVICE g_SVGA = { 0 };

VOID
SvDriverInitialize(

)
{

	for ( ULONG32 i = 0; i < HalPciDeviceList.DeviceCount; i++ ) {

		if ( HalPciDeviceList.PciDevices[ i ].PciHeader.VendorId == PCI_VENDOR_ID_VMWARE &&
			HalPciDeviceList.PciDevices[ i ].PciHeader.DeviceId == PCI_DEVICE_ID_VMWARE_SVGA2 ) {

			g_SVGA.PciDevice = &HalPciDeviceList.PciDevices[ i ];

			break;
		}
	}

	if ( !g_SVGA.PciDevice ) {

		return;
	}

	HalPciSetIoEnable( g_SVGA.PciDevice, TRUE );

	PCI_BASE_ADDRESS_REGISTER Bar[ 3 ];

	for ( ULONG32 i = 0; i < 3; i++ ) {

		HalPciReadBar( g_SVGA.PciDevice, &Bar[ i ], ( UCHAR )i );
	}

	g_SVGA.IoBase = ( USHORT )Bar[ 0 ].Base;

	g_SVGA.VideoMemoryLength = SvReadRegister( SVGA_REG_VRAM_SIZE );
	g_SVGA.FramebufferLength = SvReadRegister( SVGA_REG_FB_SIZE );
	g_SVGA.FifoLength = SvReadRegister( SVGA_REG_MEM_SIZE );

	g_SVGA.FramebufferBase = MmAllocateMemoryAtPhysical( Bar[ 1 ].Base, g_SVGA.FramebufferLength, PAGE_READ | PAGE_WRITE );
	g_SVGA.FifoBase = MmAllocateMemoryAtPhysical( Bar[ 2 ].Base, g_SVGA.FifoLength, PAGE_READ | PAGE_WRITE );

	g_SVGA.DeviceVersionId = SVGA_ID_2;

	do {

		SvWriteRegister( SVGA_REG_ID, g_SVGA.DeviceVersionId );
		if ( SvReadRegister( SVGA_REG_ID ) == g_SVGA.DeviceVersionId ) {

			break;
		}
		else {

			g_SVGA.DeviceVersionId--;
		}

	} while ( g_SVGA.DeviceVersionId >= SVGA_ID_0 );

	if ( g_SVGA.DeviceVersionId < SVGA_ID_0 ) {

		return;
	}

	//intr.

	g_SVGA.FifoBase[ SVGA_FIFO_MIN ] = SVGA_FIFO_NUM_REGS * sizeof( ULONG32 );
	g_SVGA.FifoBase[ SVGA_FIFO_MAX ] = g_SVGA.FifoLength;
	g_SVGA.FifoBase[ SVGA_FIFO_NEXT_CMD ] = g_SVGA.FifoBase[ SVGA_FIFO_MIN ];
	g_SVGA.FifoBase[ SVGA_FIFO_STOP ] = g_SVGA.FifoBase[ SVGA_FIFO_MIN ];

	if ( SvFifoCapabilityValid( SVGA_CAP_EXTENDED_FIFO ) &&
		SvFifoRegisterValid( SVGA_FIFO_GUEST_3D_HWVERSION ) ) {

		g_SVGA.FifoBase[ SVGA_FIFO_GUEST_3D_HWVERSION ] = SVGA3D_HWVERSION_CURRENT;
	}

	SvWriteRegister( SVGA_REG_ENABLE, TRUE );
	SvWriteRegister( SVGA_REG_CONFIG_DONE, TRUE );
}

VOID
SvSetMode(
	__in ULONG32 Width,
	__in ULONG32 Height,
	__in ULONG32 Bpp
)
{
	g_SVGA.Width = Width;
	g_SVGA.Height = Height;
	g_SVGA.Bpp = Bpp;

	SvWriteRegister( SVGA_REG_WIDTH, Width );
	SvWriteRegister( SVGA_REG_HEIGHT, Height );
	SvWriteRegister( SVGA_REG_BITS_PER_PIXEL, Bpp );
	SvWriteRegister( SVGA_REG_ENABLE, TRUE );
	g_SVGA.Pitch = SvReadRegister( SVGA_REG_BYTES_PER_LINE );
}

ULONG32
SvReadRegister(
	__in ULONG32 Register
)
{

	__outdword( g_SVGA.IoBase + SVGA_INDEX_PORT, Register );
	return __indword( g_SVGA.IoBase + SVGA_VALUE_PORT );
}


VOID
SvWriteRegister(
	__in ULONG32 Register,
	__in ULONG32 Value
)
{

	__outdword( g_SVGA.IoBase + SVGA_INDEX_PORT, Register );
	__outdword( g_SVGA.IoBase + SVGA_VALUE_PORT, Value );
}

BOOLEAN
SvFifoRegisterValid(
	__in ULONG32 Register
)
{
	return g_SVGA.FifoBase[ SVGA_FIFO_MIN ] > ( Register << 2 );
}

BOOLEAN
SvFifoCapabilityValid(
	__in ULONG32 Capability
)
{
	return ( g_SVGA.FifoBase[ SVGA_FIFO_CAPABILITIES ] & Capability ) != 0;
}

PVOID
SvFifoReserve(
	__in ULONG32 ByteCount
)
{

	ULONG32 Max = g_SVGA.FifoBase[ SVGA_FIFO_MAX ];
	ULONG32 Min = g_SVGA.FifoBase[ SVGA_FIFO_MIN ];
	ULONG32 NextCmd = g_SVGA.FifoBase[ SVGA_FIFO_NEXT_CMD ];

	BOOLEAN Reserveable = SvFifoCapabilityValid( SVGA_FIFO_CAP_RESERVE );

	if ( ByteCount > sizeof( g_SVGA.FifoReserve.BounceBuffer ) ||
		ByteCount > ( Max - Min ) ) {

		return NULL;
	}

	if ( ByteCount % sizeof( ULONG32 ) ) {

		return NULL;
	}

	if ( g_SVGA.FifoReserve.ReservedSize != 0 ) {

		return NULL;
	}

	g_SVGA.FifoReserve.ReservedSize = ByteCount;

	while ( 1 ) {
		ULONG32 Stop = g_SVGA.FifoBase[ SVGA_FIFO_STOP ];
		BOOLEAN ReserveInPlace = FALSE;
		BOOLEAN NeedBounce = FALSE;

		if ( NextCmd >= Stop ) {

			if ( NextCmd + ByteCount < Max ||
				( NextCmd + ByteCount == Max && Stop > Min ) ) {

				ReserveInPlace = TRUE;
			}
			else if ( ( Max - NextCmd ) + ( Stop - Min ) <= ByteCount ) {

				//fifofull
			}
			else {

				NeedBounce = TRUE;
			}
		}
		else {

			if ( NextCmd + ByteCount < Stop ) {

				ReserveInPlace = TRUE;
			}
			else {

				//fifofull
			}

		}

		if ( ReserveInPlace ) {

			if ( Reserveable || ByteCount <= sizeof( ULONG32 ) ) {

				g_SVGA.FifoReserve.UsingBounceBuffer = FALSE;

				if ( Reserveable ) {

					g_SVGA.FifoBase[ SVGA_FIFO_RESERVED ] = ByteCount;
				}

				return NextCmd + ( char* )g_SVGA.FifoBase;
			}
			else {

				NeedBounce = TRUE;
			}
		}

		if ( NeedBounce ) {

			g_SVGA.FifoReserve.UsingBounceBuffer = TRUE;

			return g_SVGA.FifoReserve.BounceBuffer;
		}

	}
}

VOID
SvFifoCommit(
	__in ULONG32 ByteCount
)
{

	ULONG32 Max = g_SVGA.FifoBase[ SVGA_FIFO_MAX ];
	ULONG32 Min = g_SVGA.FifoBase[ SVGA_FIFO_MIN ];
	ULONG32 NextCmd = g_SVGA.FifoBase[ SVGA_FIFO_NEXT_CMD ];

	BOOLEAN Reserveable = SvFifoCapabilityValid( SVGA_FIFO_CAP_RESERVE );

	if ( g_SVGA.FifoReserve.ReservedSize == 0 ) {

		return;
	}

	g_SVGA.FifoReserve.ReservedSize = 0;

	if ( g_SVGA.FifoReserve.UsingBounceBuffer ) {

		UCHAR* Buffer = g_SVGA.FifoReserve.BounceBuffer;

		if ( Reserveable ) {

			ULONG32 ChunkSize = min( ByteCount, Max - NextCmd );
			g_SVGA.FifoBase[ SVGA_FIFO_RESERVED ] = ByteCount;
			_memcpy( NextCmd + ( char* )g_SVGA.FifoBase, Buffer, ChunkSize );
			_memcpy( Min + ( char* )g_SVGA.FifoBase, Buffer + ChunkSize, ByteCount - ChunkSize );
		}
		else {

			ULONG32* Dword = ( ULONG32* )Buffer;

			while ( ByteCount > 0 ) {

				g_SVGA.FifoBase[ NextCmd / sizeof( ULONG32 ) ] = *Dword++;
				NextCmd += sizeof( ULONG32 );

				if ( NextCmd == Max ) {

					NextCmd = Min;
				}

				g_SVGA.FifoBase[ SVGA_FIFO_NEXT_CMD ] = NextCmd;
				ByteCount -= sizeof( ULONG32 );
			}
		}
	}

	if ( !g_SVGA.FifoReserve.UsingBounceBuffer ||
		Reserveable ) {

		NextCmd += ByteCount;
		if ( NextCmd >= Max ) {

			NextCmd -= Max - Min;
		}

		g_SVGA.FifoBase[ SVGA_FIFO_NEXT_CMD ] = NextCmd;
	}

	if ( Reserveable ) {

		g_SVGA.FifoBase[ SVGA_FIFO_RESERVED ] = 0;
	}

}

VOID
SvFifoCommitAll(

)
{

	SvFifoCommit( g_SVGA.FifoReserve.ReservedSize );
}

PVOID
SvFifoReserveCmd(
	__in ULONG32 Type,
	__in ULONG32 ByteCount
)
{
	ULONG32* Cmd = SvFifoReserve( ByteCount + sizeof( ULONG32 ) );
	Cmd[ 0 ] = Type;
	return &Cmd[ 1 ];
}

VOID
SvFifoFull(

)
{
	//lol
}

VOID
SvUpdate(
	__in ULONG32 x,
	__in ULONG32 y,
	__in ULONG32 Width,
	__in ULONG32 Height
)
{

	SVGAFifoCmdUpdate* cmd = SvFifoReserveCmd( SVGA_CMD_UPDATE, sizeof( SVGAFifoCmdUpdate ) );
	cmd->x = x;
	cmd->y = y;
	cmd->width = Width;
	cmd->height = Height;
	SvFifoCommitAll( );
}

VOID
SvBeginDefineCursor(
	__in SVGAFifoCmdDefineCursor* CursorInfo,
	__out PVOID* AndMask,
	__out PVOID* XorMask
)
{
	ULONG32 AndPitch = ( ( CursorInfo->andMaskDepth * CursorInfo->width + 31 ) >> 5 ) << 2;
	ULONG32 AndSize = AndPitch * CursorInfo->height;
	ULONG32 XorPitch = ( ( CursorInfo->xorMaskDepth * CursorInfo->width + 31 ) >> 5 ) << 2;
	ULONG32 XorSize = XorPitch * CursorInfo->height;

	SVGAFifoCmdDefineCursor* cmd = SvFifoReserveCmd( SVGA_CMD_DEFINE_CURSOR, sizeof( SVGAFifoCmdDefineCursor ) + AndSize + XorSize );

	*cmd = *CursorInfo;
	//_memcpy( ( void* )cmd, ( void* )CursorInfo, sizeof( SVGAFifoCmdDefineCursor ) );
	*AndMask = ( PVOID )( cmd + 1 );
	*XorMask = ( PVOID )( ( ( char* )*AndMask ) + AndSize );
}

VOID
SvBeginDefineAlphaCursor(
	__in SVGAFifoCmdDefineAlphaCursor* CursorInfo,
	__out PVOID* Cursor
)
{
	ULONG32 ImageSize = CursorInfo->width * CursorInfo->height * sizeof( ULONG32 );
	SVGAFifoCmdDefineAlphaCursor* cmd = SvFifoReserveCmd( SVGA_CMD_DEFINE_ALPHA_CURSOR, sizeof( SVGAFifoCmdDefineAlphaCursor ) + ImageSize );

	*cmd = *CursorInfo;
	*Cursor = ( PVOID )( cmd + 1 );
}

VOID
SvMoveCursor(
	__in ULONG32 Visible,
	__in ULONG32 x,
	__in ULONG32 y
)
{

	if ( SvFifoCapabilityValid( SVGA_FIFO_CAP_CURSOR_BYPASS_3 ) ) {

		g_SVGA.FifoBase[ SVGA_FIFO_CURSOR_ON ] = Visible;
		g_SVGA.FifoBase[ SVGA_FIFO_CURSOR_X ] = x;
		g_SVGA.FifoBase[ SVGA_FIFO_CURSOR_Y ] = y;
		g_SVGA.FifoBase[ SVGA_FIFO_CURSOR_COUNT ]++;
	}

}

ULONG32
SvFenceInsert(

)
{

	ULONG32 Fence;

	struct {
		ULONG32 Id;
		ULONG32 Fence;
	} *cmd;

	if ( !SvFifoCapabilityValid( SVGA_FIFO_CAP_FENCE ) ) {

		return 1;
	}

	if ( g_SVGA.FifoReserve.NextFence == 0 ) {

		g_SVGA.FifoReserve.NextFence = 1;
	}

	Fence = g_SVGA.FifoReserve.NextFence++;

	cmd = SvFifoReserve( sizeof( *cmd ) );
	cmd->Id = SVGA_CMD_FENCE;
	cmd->Fence = Fence;

	SvFifoCommitAll( );

	return Fence;
}

BOOLEAN
SvFencePassed(
	__in ULONG32 Fence
)
{

	if ( !Fence ) {

		return TRUE;
	}

	if ( !SvFifoCapabilityValid( SVGA_FIFO_CAP_FENCE ) ) {

		return FALSE;
	}

	return ( ( ULONG32 )( g_SVGA.FifoBase[ SVGA_FIFO_FENCE ] - Fence ) ) >= 0;
}

VOID
SvFenceSync(
	__in ULONG32 Fence
)
{

	if ( !Fence ) {

		return;
	}

	if ( !SvFifoCapabilityValid( SVGA_FIFO_CAP_FENCE ) ) {

		SvWriteRegister( SVGA_REG_SYNC, 1 );

		while ( SvReadRegister( SVGA_REG_BUSY ) != FALSE )
			;

		return;
	}

	if ( SvFencePassed( Fence ) ) {

		return;
	}

	BOOLEAN Busy = TRUE;

	SvWriteRegister( SVGA_REG_SYNC, 1 );

	while ( !SvFencePassed( Fence ) && Busy ) {

		Busy = SvReadRegister( SVGA_REG_BUSY ) != 0;
	}

	return;
}
