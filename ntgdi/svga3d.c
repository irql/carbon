

#include "driver.h"

#include "svga.h"
#include "svga3d.h"


VOID
SvDriver3dInitialize(

)
{

	SVGA3dHardwareVersion HardwareVersion;

	if ( !( g_SVGA.DeviceCapabilities & SVGA_CAP_EXTENDED_FIFO ) ) {

		return;
	}

	if ( SvFifoCapabilityValid( SVGA_FIFO_CAP_3D_HWVERSION_REVISED ) ) {

		HardwareVersion = g_SVGA.FifoBase[ SVGA_FIFO_3D_HWVERSION_REVISED ];
	}
	else {

		if ( g_SVGA.FifoBase[ SVGA_FIFO_MIN ] <= sizeof( ULONG32 ) * SVGA_FIFO_GUEST_3D_HWVERSION ) {

			return;
		}

		HardwareVersion = g_SVGA.FifoBase[ SVGA_FIFO_3D_HWVERSION ];
	}

	if ( HardwareVersion == 0 ) {

		return;
	}

	if ( HardwareVersion < SVGA3D_HWVERSION_WS65_B1 ) {

		return;
	}

}

PVOID
Sv3dFifoReserveCmd(
	__in ULONG32 Type,
	__in ULONG32 ByteCount
)
{

	SVGA3dCmdHeader* Header;

	Header = SvFifoReserve( sizeof( SVGA3dCmdHeader ) + ByteCount );
	Header->id = Type;
	Header->size = ByteCount;

	return &Header[ 1 ];
}

