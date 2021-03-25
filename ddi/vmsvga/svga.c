


#include "driver.h"
#include "svga.h"
#include "svga3d.h"
#include "ddi.h"

int _fltused = 0;

//
//  a lot of code is ripped from the vmware reference driver
//  https://github.com/prepare/vmware-svga/blob/master/lib/refdriver/
//

ULONG64 DdAdapterCount = 0;

BOOLEAN
DdSvgaDevice(
    _In_ PUNICODE_STRING LinkName,
    _In_ PDEVICE_OBJECT  DeviceObject,
    _In_ PDRIVER_OBJECT  DriverObject
)
{
    DriverObject;
    LinkName;
    PPCI_DEVICE PciSvga;
    PCI_BASE Bar[ 3 ];
    ULONG32 CurrentBar;
    PSVGA_DEVICE Extension;
    PDEVICE_OBJECT SvgaDeviceObject;
    PDEVICE_OBJECT DdiDeviceObject;
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING( L"\\Device\\VMSVGA0" );
    PD3DHAL D3dHal;

    PciSvga = DeviceObject->DeviceExtension;

    IoCreateDevice( DriverObject,
                    sizeof( SVGA_DEVICE ),
                    &DeviceName,
                    0u,
                    &SvgaDeviceObject );

    Extension = DdGetDevice( SvgaDeviceObject );

    SvgaDeviceObject->DeviceCharacteristics &= DEV_INITIALIZING;

    PciSetIoEnable( PciSvga, TRUE );

    for ( CurrentBar = 0; CurrentBar < 3; CurrentBar++ ) {

        PciReadBase( PciSvga, Bar + CurrentBar, CurrentBar );
    }

    Extension->IoBase = ( USHORT )Bar[ 0 ].Base;
    Extension->VideoMemoryLength = DdReadRegister( Extension, SVGA_REG_VRAM_SIZE );
    Extension->FramebufferLength = DdReadRegister( Extension, SVGA_REG_FB_SIZE );
    Extension->FifoLength = DdReadRegister( Extension, SVGA_REG_MEM_SIZE );

    Extension->FramebufferBase = MmMapIoSpace( Bar[ 1 ].Base, Extension->FramebufferLength );
    Extension->FifoBase = MmMapIoSpace( Bar[ 2 ].Base, Extension->FifoLength );

    Extension->DeviceVersionId = SVGA_ID_2;

    do {

        DdWriteRegister( Extension, SVGA_REG_ID, Extension->DeviceVersionId );
        if ( DdReadRegister( Extension, SVGA_REG_ID ) == Extension->DeviceVersionId ) {

            break;
        }
        else {

            Extension->DeviceVersionId--;
        }

    } while ( Extension->DeviceVersionId >= SVGA_ID_0 );

    if ( Extension->DeviceVersionId < SVGA_ID_0 ) {

        return TRUE;
    }

    //intr

    Extension->DeviceCapabilities = DdReadRegister( Extension, SVGA_REG_CAPABILITIES );
    Extension->FifoBase[ SVGA_FIFO_MIN ] = SVGA_FIFO_NUM_REGS * sizeof( ULONG32 );
    Extension->FifoBase[ SVGA_FIFO_MAX ] = Extension->FifoLength;
    Extension->FifoBase[ SVGA_FIFO_NEXT_CMD ] = Extension->FifoBase[ SVGA_FIFO_MIN ];
    Extension->FifoBase[ SVGA_FIFO_STOP ] = Extension->FifoBase[ SVGA_FIFO_MIN ];

    if ( DdFifoCapabilityValid( Extension, SVGA_CAP_EXTENDED_FIFO ) &&
         DdFifoRegisterValid( Extension, SVGA_FIFO_GUEST_3D_HWVERSION ) ) {

        Extension->FifoBase[ SVGA_FIFO_GUEST_3D_HWVERSION ] = SVGA3D_HWVERSION_WS65_B1;//SVGA3D_HWVERSION_CURRENT;
        //Extension->FifoBase[ SVGA_FIFO_3D_HWVERSION ] = SVGA3D_HWVERSION_WS65_B1;
    }

    DdWriteRegister( Extension, SVGA_REG_ENABLE, TRUE );
    DdWriteRegister( Extension, SVGA_REG_CONFIG_DONE, TRUE );

    //
    // This is where we should call into dxgi and setup our ddi
    //

    KdPrint( L"svga device found and initialized %ull %ull\n", Extension->FramebufferLength, Extension->FifoLength );

    DdSetMode( Extension, 1280, 720, 32 );
    Dd3dInitialize( Extension );

    //DdUpdate( Extension, 0, 0, 1280, 720 );

    DdCreateAdapter( SvgaDeviceObject,
                     &DdiDeviceObject );

    //
    // Make a characteristic for deviceobjects so that 
    // user mode shit can send requests to this with different
    // major codes n shit (maybe not actually)
    //
    // Fill in d3dhal
    //

    D3dHal = DdGetAdapterD3dHal( DdiDeviceObject );
    D3dHal->D3dHalVersion = D3DHAL_VERSION_01_00;
    D3dHal->D3dHalSurfaceCreate = D3dSurfaceCreate;
    D3dHal->D3dHalContextCreate = D3dContextCreate;
    D3dHal->D3dHalClear = D3dClear;
    D3dHal->D3dHalSetRenderTarget = D3dSetRenderTarget;
    D3dHal->D3dHalSetViewport = D3dSetViewport;
    D3dHal->D3dHalSetZRange = D3dSetZRange;
    D3dHal->D3dHalSetRenderState = D3dSetRenderState;
    D3dHal->D3dHalPresent = D3dPresent;
    D3dHal->D3dHalSubmitCommand = D3dSubmitCommand;

    DdiDeviceObject->DeviceCharacteristics &= ~DEV_INITIALIZING;
#if 0
    ULONG32 ImageSurface = 0;
    ULONG32 DepthSurface = 0;
    ULONG32 ScreenContext = 0;

    D3DHAL_DATA_SURFACE_CREATE SurfaceCreate = { 0 };
    D3DHAL_DATA_SET_RENDER_TARGET RenderTarget = { 0 };
    D3DHAL_DATA_CONTEXT_CREATE ContextCreate = { 0 };
    D3DHAL_DATA_SET_RENDER_STATE RenderState = { 0 };
    D3DHAL_DATA_SET_ZRANGE ZRange = { 0 };
    D3DHAL_DATA_SET_VIEWPORT Viewport = { 0 };
    D3DHAL_DATA_PRESENT Present = { 0 };
    D3DHAL_DATA_CLEAR Clear = { 0 };

    SurfaceCreate.Flags = 0;
    SurfaceCreate.Format = D3DDDI_X8R8G8B8;
    SurfaceCreate.Width = 1280;
    SurfaceCreate.Height = 720;
    SurfaceCreate.Depth = 1;
    D3dSurfaceCreate( DdiDeviceObject->DeviceLink, &SurfaceCreate );
    ImageSurface = SurfaceCreate.SurfaceId;

    RtlDebugPrint( L"first done.\n" );

    SurfaceCreate.Flags = 0;
    SurfaceCreate.Width = 1280;
    SurfaceCreate.Height = 720;
    SurfaceCreate.Depth = 1;
    SurfaceCreate.Format = D3DDDI_Z_D16;
    D3dSurfaceCreate( DdiDeviceObject->DeviceLink, &SurfaceCreate );
    DepthSurface = SurfaceCreate.SurfaceId;

    RtlDebugPrint( L"second done.\n" );

    D3dContextCreate( DdiDeviceObject->DeviceLink, &ContextCreate );
    ScreenContext = ContextCreate.ContextId;

    RenderTarget.ContextId = ScreenContext;
    RenderTarget.Type = D3DDDI_RT_COLOR0;
    RenderTarget.SurfaceId = ImageSurface;
    D3dSetRenderTarget( DdiDeviceObject->DeviceLink, &RenderTarget );

    RenderTarget.ContextId = ScreenContext;
    RenderTarget.Type = D3DDDI_RT_DEPTH;
    RenderTarget.SurfaceId = DepthSurface;
    D3dSetRenderTarget( DdiDeviceObject->DeviceLink, &RenderTarget );

    Viewport.ContextId = ScreenContext;
    Viewport.x = 0;
    Viewport.y = 0;
    Viewport.w = 1280;
    Viewport.h = 720;
    D3dSetViewport( DdiDeviceObject->DeviceLink, &Viewport );

    ZRange.ContextId = ScreenContext;
    ZRange.Minimum = 0.0f;
    ZRange.Maximum = 1.0f;
    D3dSetZRange( DdiDeviceObject->DeviceLink, &ZRange );

    RenderState.ContextId = ScreenContext;
    RenderState.StateCount = 1;
    RenderState.States[ 0 ].State = D3DDDI_RS_SHADEMODE;
    RenderState.States[ 0 ].Long = D3DDDI_SHADEMODE_SMOOTH;
    D3dSetRenderState( DdiDeviceObject->DeviceLink, &RenderState );

    Clear.ContextId = ScreenContext;
    Clear.x = 0;
    Clear.y = 0;
    Clear.w = 1280;
    Clear.h = 720;
    Clear.Colour = 0x113366;
    Clear.Depth = 1.0f;
    Clear.Stencil = 0;
    Clear.Flags = D3DDDI_CLEAR_COLOR | D3DDDI_CLEAR_DEPTH;
    D3dClear( DdiDeviceObject->DeviceLink, &Clear );

    Present.SurfaceId = ImageSurface;
    Present.x = 0;
    Present.y = 0;
    Present.w = 1280;
    Present.h = 720;
    D3dPresent( DdiDeviceObject->DeviceLink, &Present );

#endif
    //DdUpdate( Extension, 0, 0, 1280, 720 );
    return TRUE;
}

VOID
DdDriverInitialize(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    PciQueryDevices(
        L"\\??\\PCI#VEN_15AD&DEV_0405&{??,??,??}&{??,??,??}",
        ( PKPCI_QUERY_DEVICE )DdSvgaDevice, DriverObject );


}

VOID
DdSetMode(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      Width,
    _In_ ULONG32      Height,
    _In_ ULONG32      Bpp
)
{
    Svga->Width = Width;
    Svga->Height = Height;
    Svga->Bpp = Bpp;

    DdWriteRegister( Svga, SVGA_REG_WIDTH, Width );
    DdWriteRegister( Svga, SVGA_REG_HEIGHT, Height );
    DdWriteRegister( Svga, SVGA_REG_BITS_PER_PIXEL, Bpp );
    DdWriteRegister( Svga, SVGA_REG_ENABLE, TRUE );
    Svga->Pitch = DdReadRegister( Svga, SVGA_REG_BYTES_PER_LINE );
}

ULONG32
DdReadRegister(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      Register
)
{

    __outdword( Svga->IoBase + SVGA_INDEX_PORT, Register );
    return __indword( Svga->IoBase + SVGA_VALUE_PORT );
}


VOID
DdWriteRegister(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      Register,
    _In_ ULONG32      Value
)
{

    __outdword( Svga->IoBase + SVGA_INDEX_PORT, Register );
    __outdword( Svga->IoBase + SVGA_VALUE_PORT, Value );
}

BOOLEAN
DdFifoRegisterValid(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      Register
)
{
    return Svga->FifoBase[ SVGA_FIFO_MIN ] > ( Register * sizeof( ULONG32 ) );
}

BOOLEAN
DdFifoCapabilityValid(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      Capability
)
{
    return ( Svga->FifoBase[ SVGA_FIFO_CAPABILITIES ] & Capability ) != 0;
}

PVOID
DdFifoReserve(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      ByteCount
)
{

    ULONG32 Max = Svga->FifoBase[ SVGA_FIFO_MAX ];
    ULONG32 Min = Svga->FifoBase[ SVGA_FIFO_MIN ];
    ULONG32 NextCmd = Svga->FifoBase[ SVGA_FIFO_NEXT_CMD ];

    BOOLEAN Reserveable = DdFifoCapabilityValid( Svga, SVGA_FIFO_CAP_RESERVE );

    if ( ByteCount > sizeof( Svga->FifoReserve.BounceBuffer ) ||
         ByteCount > ( Max - Min ) ) {
        RtlDebugPrint( L"cmd too large" );
        return NULL;
    }

    if ( ByteCount % sizeof( ULONG32 ) ) {
        RtlDebugPrint( L"align to 4b\n" );
        return NULL;
    }

    if ( Svga->FifoReserve.ReservedSize != 0 ) {
        RtlDebugPrint( L"fifo res before commit" );
        return NULL;
    }

    Svga->FifoReserve.ReservedSize = ByteCount;

    while ( 1 ) {
        ULONG32 Stop = Svga->FifoBase[ SVGA_FIFO_STOP ];
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

                Svga->FifoReserve.UsingBounceBuffer = FALSE;

                if ( Reserveable ) {

                    Svga->FifoBase[ SVGA_FIFO_RESERVED ] = ByteCount;
                }

                return NextCmd + ( char* )Svga->FifoBase;
            }
            else {

                NeedBounce = TRUE;
            }
        }

        if ( NeedBounce ) {

            Svga->FifoReserve.UsingBounceBuffer = TRUE;

            return Svga->FifoReserve.BounceBuffer;
        }

    }
}

VOID
DdFifoCommit(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      ByteCount
)
{

    ULONG32 Max = Svga->FifoBase[ SVGA_FIFO_MAX ];
    ULONG32 Min = Svga->FifoBase[ SVGA_FIFO_MIN ];
    ULONG32 NextCmd = Svga->FifoBase[ SVGA_FIFO_NEXT_CMD ];

    BOOLEAN Reserveable = DdFifoCapabilityValid( Svga, SVGA_FIFO_CAP_RESERVE );

    if ( Svga->FifoReserve.ReservedSize == 0 ) {
        RtlDebugPrint( L"Commit before reserve\n" );
        return;
    }

    Svga->FifoReserve.ReservedSize = 0;

    if ( Svga->FifoReserve.UsingBounceBuffer ) {

        UCHAR* Buffer = Svga->FifoReserve.BounceBuffer;

        if ( Reserveable ) {

            ULONG32 ChunkSize = min( ByteCount, Max - NextCmd );
            Svga->FifoBase[ SVGA_FIFO_RESERVED ] = ByteCount;
            RtlCopyMemory( ( PUCHAR )Svga->FifoBase + NextCmd, Buffer, ChunkSize );
            RtlCopyMemory( ( PUCHAR )Svga->FifoBase + Min, Buffer + ChunkSize, ByteCount - ChunkSize );
        }
        else {

            ULONG32* Dword = ( ULONG32* )Buffer;

            while ( ByteCount > 0 ) {

                Svga->FifoBase[ NextCmd / sizeof( ULONG32 ) ] = *Dword++; // pf here
                NextCmd += sizeof( ULONG32 );

                if ( NextCmd == Max ) {

                    NextCmd = Min;
                }

                Svga->FifoBase[ SVGA_FIFO_NEXT_CMD ] = NextCmd;
                ByteCount -= sizeof( ULONG32 );
            }
        }
    }

    if ( !Svga->FifoReserve.UsingBounceBuffer ||
         Reserveable ) {

        NextCmd += ByteCount;
        if ( NextCmd >= Max ) {

            NextCmd -= Max - Min;
        }

        Svga->FifoBase[ SVGA_FIFO_NEXT_CMD ] = NextCmd;
    }

    if ( Reserveable ) {

        Svga->FifoBase[ SVGA_FIFO_RESERVED ] = 0;
    }

}

VOID
DdFifoCommitAll(
    _In_ PSVGA_DEVICE Svga
)
{

    DdFifoCommit( Svga, Svga->FifoReserve.ReservedSize );
}

PVOID
DdFifoReserveCmd(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      Type,
    _In_ ULONG32      ByteCount
)
{
    ULONG32* Cmd = DdFifoReserve( Svga, ByteCount + sizeof( ULONG32 ) );
    Cmd[ 0 ] = Type;
    return &Cmd[ 1 ];
}

VOID
DdFifoFull(

)
{
    //lol
}

VOID
DdUpdate(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      x,
    _In_ ULONG32      y,
    _In_ ULONG32      Width,
    _In_ ULONG32      Height
)
{

    SVGAFifoCmdUpdate* cmd = DdFifoReserveCmd( Svga, SVGA_CMD_UPDATE, sizeof( SVGAFifoCmdUpdate ) );
    cmd->x = x;
    cmd->y = y;
    cmd->width = Width;
    cmd->height = Height;
    DdFifoCommitAll( Svga );
}

#if 0
VOID
SvBeginDefineCursor(
    _In_  SVGAFifoCmdDefineCursor* CursorInfo,
    _Out_ PVOID* AndMask,
    _Out_ PVOID* XorMask
)
{
    ULONG32 AndPitch = ( ( CursorInfo->andMaskDepth * CursorInfo->width + 31 ) >> 5 ) << 2;
    ULONG32 AndSize = AndPitch * CursorInfo->height;
    ULONG32 XorPitch = ( ( CursorInfo->xorMaskDepth * CursorInfo->width + 31 ) >> 5 ) << 2;
    ULONG32 XorSize = XorPitch * CursorInfo->height;

    SVGAFifoCmdDefineCursor* cmd = DdFifoReserveCmd( SVGA_CMD_DEFINE_CURSOR, sizeof( SVGAFifoCmdDefineCursor ) + AndSize + XorSize );

    *cmd = *CursorInfo;
    //_memcpy( ( void* )cmd, ( void* )CursorInfo, sizeof( SVGAFifoCmdDefineCursor ) );
    *AndMask = ( PVOID )( cmd + 1 );
    *XorMask = ( PVOID )( ( ( char* )*AndMask ) + AndSize );
}

VOID
SvBeginDefineAlphaCursor(
    _In_ SVGAFifoCmdDefineAlphaCursor* CursorInfo,
    _Out_ PVOID* Cursor
)
{
    ULONG32 ImageSize = CursorInfo->width * CursorInfo->height * sizeof( ULONG32 );
    SVGAFifoCmdDefineAlphaCursor* cmd = DdFifoReserveCmd( SVGA_CMD_DEFINE_ALPHA_CURSOR, sizeof( SVGAFifoCmdDefineAlphaCursor ) + ImageSize );

    *cmd = *CursorInfo;
    *Cursor = ( PVOID )( cmd + 1 );
}

VOID
SvMoveCursor(
    _In_ ULONG32 Visible,
    _In_ ULONG32 x,
    _In_ ULONG32 y
)
{

    if ( DdFifoCapabilityValid( SVGA_FIFO_CAP_CURSOR_BYPASS_3 ) ) {

        g_SVGA.FifoBase[ SVGA_FIFO_CURSOR_ON ] = Visible;
        g_SVGA.FifoBase[ SVGA_FIFO_CURSOR_X ] = x;
        g_SVGA.FifoBase[ SVGA_FIFO_CURSOR_Y ] = y;
        g_SVGA.FifoBase[ SVGA_FIFO_CURSOR_COUNT ]++;
    }

}
#endif

ULONG32
DdFenceInsert(
    _In_ PSVGA_DEVICE Svga
)
{

    ULONG32 Fence;

    struct {
        ULONG32 Id;
        ULONG32 Fence;
    } *cmd;

    if ( !DdFifoCapabilityValid( Svga, SVGA_FIFO_CAP_FENCE ) ) {

        return 1;
    }

    if ( Svga->FifoReserve.NextFence == 0 ) {

        Svga->FifoReserve.NextFence = 1;
    }

    Fence = Svga->FifoReserve.NextFence++;

    cmd = DdFifoReserve( Svga, sizeof( *cmd ) );
    cmd->Id = SVGA_CMD_FENCE;
    cmd->Fence = Fence;

    DdFifoCommitAll( Svga );

    return Fence;
}

BOOLEAN
DdFencePassed(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      Fence
)
{

    if ( !Fence ) {

        return TRUE;
    }

    if ( !DdFifoCapabilityValid( Svga, SVGA_FIFO_CAP_FENCE ) ) {

        return FALSE;
    }

    return ( ( ULONG32 )( Svga->FifoBase[ SVGA_FIFO_FENCE ] - Fence ) ) >= 0;
}

VOID
DdFenceSync(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      Fence
)
{

    if ( !Fence ) {

        return;
    }

    if ( !DdFifoCapabilityValid( Svga, SVGA_FIFO_CAP_FENCE ) ) {

        DdWriteRegister( Svga, SVGA_REG_SYNC, 1 );

        while ( DdReadRegister( Svga, SVGA_REG_BUSY ) != FALSE )
            ;

        return;
    }

    if ( DdFencePassed( Svga, Fence ) ) {

        return;
    }

    BOOLEAN Busy = TRUE;

    DdWriteRegister( Svga, SVGA_REG_SYNC, 1 );

    while ( !DdFencePassed( Svga, Fence ) && Busy ) {

        Busy = DdReadRegister( Svga, SVGA_REG_BUSY ) != 0;
    }

    return;
}
