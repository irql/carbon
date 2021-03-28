


#include "driver.h"
#include "svga.h"
#include "svga3d.h"
#include "ddi.h"

typedef struct _DDI_SVGA_DC_CONTEXT {
    ULONG32 SurfaceId;
    ULONG32 ContextId;

} DDI_SVGA_DC_CONTEXT, *PDDI_SVGA_DC_CONTEXT;

#define DdContextDC( dc ) ( ( PDDI_SVGA_DC_CONTEXT )( ( PDC )( dc ) )->DeviceSpecific ) 

VOID
NtDdiSvgaInitializeDC(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PDC            DeviceContext
)
{
    STATIC BOOLEAN ScreenDC = FALSE;

    ULONG32 Width;
    ULONG32 Height;

    PDDI_SVGA_DC_CONTEXT Ddi;
    D3DHAL_DATA_SURFACE_CREATE SurfaceCreate = { 0 };
    D3DHAL_DATA_CONTEXT_CREATE ContextCreate = { 0 };
    D3DHAL_DATA_SET_RENDER_TARGET RenderTarget = { 0 };
    D3DHAL_DATA_SET_VIEWPORT Viewport = { 0 };
    D3DHAL_DATA_SET_RENDER_STATE RenderState = { 0 };

    DeviceContext->DeviceSpecific = MmAllocatePoolWithTag( NonPagedPoolZeroed,
                                                           sizeof( DDI_SVGA_DC_CONTEXT ),
                                                           SVGA_TAG );
    Ddi = DeviceContext->DeviceSpecific;

    Width = DeviceContext->ClientArea.Right - DeviceContext->ClientArea.Left;
    Height = DeviceContext->ClientArea.Bottom - DeviceContext->ClientArea.Top;

    SurfaceCreate.Flags = 0;
    SurfaceCreate.Format = D3DDDI_X8R8G8B8;
    SurfaceCreate.Width = Width;
    SurfaceCreate.Height = Height;
    SurfaceCreate.Depth = 1;
    D3dSurfaceCreate( DeviceObject, &SurfaceCreate );
    Ddi->SurfaceId = SurfaceCreate.SurfaceId;

    D3dContextCreate( DeviceObject, &ContextCreate );
    Ddi->ContextId = ContextCreate.ContextId;

    RenderTarget.ContextId = Ddi->ContextId;
    RenderTarget.Type = D3DDDI_RT_COLOR0;
    RenderTarget.SurfaceId = Ddi->SurfaceId;
    D3dSetRenderTarget( DeviceObject, &RenderTarget );

    Viewport.ContextId = Ddi->ContextId;
    Viewport.x = 0;
    Viewport.y = 0;
    Viewport.w = Width;
    Viewport.h = Height;
    D3dSetViewport( DeviceObject, &Viewport );

    RenderState.ContextId = Ddi->ContextId;
    RenderState.StateCount = 1;
    RenderState.States[ 0 ].State = D3DDDI_RS_SHADEMODE;
    RenderState.States[ 0 ].Long = D3DDDI_SHADEMODE_SMOOTH;
    D3dSetRenderState( DeviceObject, &RenderState );

}

VOID
NtDdiSvgaReleaseDC(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PDC            DeviceContext
)
{
    DeviceObject;

    MmFreePoolWithTag( DeviceContext->DeviceSpecific, SVGA_TAG );
}

VOID
NtDdiSvgaBlt(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PDC            SourceContext,
    _In_ ULONG32        srcx,
    _In_ ULONG32        srcy,
    _In_ ULONG32        srcw,
    _In_ ULONG32        srch,
    _In_ PDC            DestinationContext,
    _In_ ULONG32        dstx,
    _In_ ULONG32        dsty
)
{
    DeviceObject;

    PDDI_SVGA_DC_CONTEXT SourceSvgaContext = DdContextDC( SourceContext );
    PDDI_SVGA_DC_CONTEXT DestinationSvgaContext = DdContextDC( DestinationContext );

    SVGA3dSurfaceImageId ImageSource = { 0 };
    SVGA3dSurfaceImageId ImageDestination = { 0 };

    SVGA3dCopyBox* Boxes;

    ImageSource.sid = SourceSvgaContext->SurfaceId;
    ImageDestination.sid = DestinationSvgaContext->SurfaceId;

    Dd3dBeginSurfaceCopy( DdGetDevice( DeviceObject ),
                          &ImageSource,
                          &ImageDestination,
                          &Boxes,
                          1 );
    Boxes[ 0 ].srcx = srcx;
    Boxes[ 0 ].srcy = srcy;
    Boxes[ 0 ].w = srcw;
    Boxes[ 0 ].h = srch;
    Boxes[ 0 ].x = dstx;
    Boxes[ 0 ].y = dsty;

    Boxes->d = 1;

    DdFifoCommitAll( DdGetDevice( DeviceObject ) );
}
#if 0
VOID
NtDdiSvgaBltBits(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PVOID          srcbits,
    _In_ ULONG32        srcx,
    _In_ ULONG32        srcy,
    _In_ ULONG32        srcw,
    _In_ ULONG32        srch,
    _In_ PDC            context,
    _In_ ULONG32        dstx,
    _In_ ULONG32        dsty
)
{
    SVGA3dGuestImage SourceImage;
    SVGA3dSurfaceImageId DestinationImage;
    SVGA3dCopyBox* Boxes;

    SourceImage.ptr = srcbits;
    SourceImage.pitch = srcw;

    Dd3dBeginSurfaceDma( DdGetDevice( DeviceObject ),
                         &SourceImage,
                         &DestinationImage,
                         SVGA3D_WRITE_HOST_VRAM,
                         &Boxes,
                         1 );

}
#endif
