


#include "driver.h"
#include "svga.h"
#include "svga3d.h"
#include "ddi.h"

NTSTATUS
D3dSurfaceCreate(
    _In_ PDEVICE_OBJECT              Device,
    _In_ PD3DHAL_DATA_SURFACE_CREATE Data
)
{
    SVGA3dSize* Sizes;
    SVGA3dSurfaceFace* Faces;

    Data->SurfaceId = DdCreateUniqueId( );
    Dd3dBeginDefineSurface( DdGetDevice( Device ),
                            Data->SurfaceId,
                            Data->Flags,
                            Data->Format,
                            &Faces,
                            &Sizes,
                            1 );

    Faces[ 0 ].numMipLevels = 1;
    Sizes[ 0 ].width = Data->Width;
    Sizes[ 0 ].height = Data->Height;
    Sizes[ 0 ].depth = Data->Depth;

    DdFifoCommitAll( DdGetDevice( Device ) );

    return DXSTATUS_SUCCESS;
}

NTSTATUS
D3dSetRenderTarget(
    _In_ PDEVICE_OBJECT                 Device,
    _In_ PD3DHAL_DATA_SET_RENDER_TARGET Data
)
{
    SVGA3dSurfaceImageId ImageId;

    ImageId.sid = Data->SurfaceId;
    ImageId.face = Data->Face;
    ImageId.mipmap = Data->Mipmap;

    Dd3dSetRenderTarget( DdGetDevice( Device ),
                         Data->ContextId,
                         Data->Type,
                         &ImageId );

    return DXSTATUS_SUCCESS;
}

NTSTATUS
D3dContextCreate(
    _In_ PDEVICE_OBJECT              Device,
    _In_ PD3DHAL_DATA_CONTEXT_CREATE Data
)
{
    Data->ContextId = DdCreateUniqueId( );
    Dd3dDefineContext( DdGetDevice( Device ),
                       Data->ContextId );

    return DXSTATUS_SUCCESS;
}

NTSTATUS
D3dSetViewport(
    _In_ PDEVICE_OBJECT            Device,
    _In_ PD3DHAL_DATA_SET_VIEWPORT Data
)
{
    SVGA3dRect Rect;

    Dd3dBeginSetViewport( DdGetDevice( Device ),
                          Data->ContextId,
                          &Rect );
    Rect.x = Data->x;
    Rect.y = Data->y;
    Rect.w = Data->w;
    Rect.h = Data->h;

    return DXSTATUS_SUCCESS;
}

NTSTATUS
D3dSetZRange(
    _In_ PDEVICE_OBJECT          Device,
    _In_ PD3DHAL_DATA_SET_ZRANGE Data
)
{
    Dd3dSetZRange( DdGetDevice( Device ),
                   Data->ContextId,
                   Data->Minimum,
                   Data->Maximum );

    return DXSTATUS_SUCCESS;
}

NTSTATUS
D3dSetRenderState(
    _In_ PDEVICE_OBJECT                Device,
    _In_ PD3DHAL_DATA_SET_RENDER_STATE Data
)
{
    SVGA3dRenderState* RenderState;

    Dd3dBeginSetRenderState( DdGetDevice( Device ),
                             Data->ContextId,
                             &RenderState,
                             Data->StateCount );

    //RtlCopyMemory( RenderState,
    //               Data->States,
    //               Data->StateCount * sizeof( Data->States ) );

    RenderState->state = SVGA3D_RS_SHADEMODE;
    RenderState->uintValue = SVGA3D_SHADEMODE_SMOOTH;

    DdFifoCommitAll( DdGetDevice( Device ) );

    return DXSTATUS_SUCCESS;
}

NTSTATUS
D3dClear(
    _In_ PDEVICE_OBJECT     Device,
    _In_ PD3DHAL_DATA_CLEAR Data
)
{
    SVGA3dRect* Rect;

    Dd3dBeginClear( DdGetDevice( Device ),
                    Data->ContextId,
                    Data->Flags,
                    Data->Colour,
                    Data->Depth,
                    Data->Stencil,
                    &Rect,
                    1 );
    Rect->x = Data->x;
    Rect->y = Data->y;
    Rect->w = Data->w;
    Rect->h = Data->h;

    DdFifoCommitAll( DdGetDevice( Device ) );

    return DXSTATUS_SUCCESS;
}

NTSTATUS
D3dPresent(
    _In_ PDEVICE_OBJECT       Device,
    _In_ PD3DHAL_DATA_PRESENT Data
)
{
    SVGA3dCopyRect* CopyRect;

    //DdFenceSync( DdGetDevice( Device ),
    //              );

    Dd3dBeginPresent( DdGetDevice( Device ),
                      Data->SurfaceId,
                      &CopyRect,
                      1 );
    CopyRect->x = Data->x;
    CopyRect->y = Data->y;
    CopyRect->w = Data->w;
    CopyRect->h = Data->h;
    CopyRect->srcx = 0;
    CopyRect->srcy = 0;

    DdFifoCommitAll( DdGetDevice( Device ) );

    //DdUpdate( DdGetDevice( Device ), 0, 0, 1280, 720 );

    return DXSTATUS_SUCCESS;
}

NTSTATUS
D3dSubmitCommand(
    _In_ PDEVICE_OBJECT Device
)
{
    DdUpdate( DdGetDevice( Device ), 0, 0, 1280, 720 );
    DdFifoCommitAll( DdGetDevice( Device ) );

    return DXSTATUS_SUCCESS;
}
