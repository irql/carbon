


#include "driver.h"
#include "svga.h"
#include "svga3d.h"

VOID
Dd3dInitialize(
    _In_ PSVGA_DEVICE Svga
)
{

    SVGA3dHardwareVersion HardwareVersion;

    if ( !( Svga->DeviceCapabilities & SVGA_CAP_EXTENDED_FIFO ) ) {

        return;
    }

    if ( DdFifoCapabilityValid( Svga, SVGA_FIFO_CAP_3D_HWVERSION_REVISED ) ) {

        HardwareVersion = Svga->FifoBase[ SVGA_FIFO_3D_HWVERSION_REVISED ];
    }
    else {

        if ( Svga->FifoBase[ SVGA_FIFO_MIN ] <= sizeof( ULONG32 ) * SVGA_FIFO_GUEST_3D_HWVERSION ) {

            return;
        }

        HardwareVersion = Svga->FifoBase[ SVGA_FIFO_3D_HWVERSION ];
    }

    //Svga->FifoBase[ SVGA_FIFO_3D_HWVERSION ] = SVGA3D_HWVERSION_WS65_B1;
    RtlDebugPrint( L"3d hw ver= %d.%d\n", HardwareVersion >> 16, HardwareVersion & 0xFF );

    if ( HardwareVersion == 0 ) {

        RtlDebugPrint( L"3d disabled by host.\n" );
        return;
    }

    if ( HardwareVersion < SVGA3D_HWVERSION_WS65_B1 ) {

        RtlDebugPrint( L"Old?\n" );
        return;
    }

}

PVOID
Dd3dFifoReserveCmd(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      Type,
    _In_ ULONG32      ByteCount
)
{
    SVGA3dCmdHeader* Header;

    Header = DdFifoReserve( Svga, sizeof( SVGA3dCmdHeader ) + ByteCount );
    Header->id = Type;
    Header->size = ByteCount;

    return Header + 1;
}

VOID
Dd3dBeginDefineSurface(
    _In_  PSVGA_DEVICE        Svga,
    _In_  ULONG32             Id,
    _In_  SVGA3dSurfaceFlags  Flags,
    _In_  SVGA3dSurfaceFormat Format,
    _Out_ SVGA3dSurfaceFace** Faces,
    _Out_ SVGA3dSize**        Sizes,
    _In_  ULONG32             SizesCount
)
{
    SVGA3dCmdDefineSurface* Command;

    Command = Dd3dFifoReserveCmd( Svga,
                                  SVGA_3D_CMD_SURFACE_DEFINE,
                                  sizeof( SVGA3dCmdDefineSurface ) +
                                  sizeof( SVGA3dSize ) * SizesCount );
    Command->sid = Id;
    Command->surfaceFlags = Flags;
    Command->format = Format;
    *Faces = Command->face;
    *Sizes = ( SVGA3dSize* )( Command + 1 );

    RtlZeroMemory( *Faces, sizeof( SVGA3dSurfaceFace ) * SVGA3D_MAX_SURFACE_FACES );
    RtlZeroMemory( *Sizes, sizeof( SVGA3dSize ) * SizesCount );
}

VOID
Dd3dDestroySurface(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      Id
)
{
    SVGA3dCmdDestroySurface* Command;

    Command = Dd3dFifoReserveCmd( Svga,
                                  SVGA_3D_CMD_SURFACE_DESTROY,
                                  sizeof( SVGA3dCmdDestroySurface ) );
    Command->sid = Id;
    DdFifoCommitAll( Svga );
}

VOID
Dd3dBeginSurfaceDma(
    _In_  PSVGA_DEVICE          Svga,
    _In_  SVGA3dGuestImage*     GuestImage,
    _In_  SVGA3dSurfaceImageId* HostImage,
    _In_  SVGA3dTransferType    Transfer,
    _Out_ SVGA3dCopyBox**       Boxes,
    _In_  ULONG32               BoxCount
)
{
    SVGA3dCmdSurfaceDMA* Command;
    ULONG32 BoxLength = sizeof( SVGA3dCopyBox ) * BoxCount;

    Command = Dd3dFifoReserveCmd( Svga,
                                  SVGA_3D_CMD_SURFACE_DMA,
                                  sizeof( SVGA3dCmdSurfaceDMA ) + BoxLength );
    Command->guest = *GuestImage;
    Command->host = *HostImage;
    Command->transfer = Transfer;
    *Boxes = ( SVGA3dCopyBox* )( Command + 1 );

    RtlZeroMemory( *Boxes, BoxLength );
}

VOID
Dd3dDefineContext(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      Id
)
{
    SVGA3dCmdDefineContext* Command;

    Command = Dd3dFifoReserveCmd( Svga,
                                  SVGA_3D_CMD_CONTEXT_DEFINE,
                                  sizeof( SVGA3dCmdDefineContext ) );
    Command->cid = Id;
    DdFifoCommitAll( Svga );
}

VOID
Dd3dDestroyContext(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      ContextId
)
{
    SVGA3dCmdDestroyContext* Command;

    Command = Dd3dFifoReserveCmd( Svga,
                                  SVGA_3D_CMD_CONTEXT_DESTROY,
                                  sizeof( SVGA3dCmdDestroyContext ) );
    Command->cid = ContextId;
    DdFifoCommitAll( Svga );
}

VOID
Dd3dSetRenderTarget(
    _In_ PSVGA_DEVICE           Svga,
    _In_ ULONG32                ContextId,
    _In_ SVGA3dRenderTargetType Type,
    _In_ SVGA3dSurfaceImageId*  Target
)
{
    SVGA3dCmdSetRenderTarget* Command;

    Command = Dd3dFifoReserveCmd( Svga,
                                  SVGA_3D_CMD_SETRENDERTARGET,
                                  sizeof( SVGA3dCmdSetRenderTarget ) );
    Command->cid = ContextId;
    Command->type = Type;
    Command->target = *Target;
    DdFifoCommitAll( Svga );
}

VOID
Dd3dSetTransform(
    _In_ PSVGA_DEVICE           Svga,
    _In_ ULONG32                ContextId,
    _In_ SVGA3dRenderTargetType Type,
    _In_ float*                 Transform
)
{
    SVGA3dCmdSetTransform* Command;

    Command = Dd3dFifoReserveCmd( Svga,
                                  SVGA_3D_CMD_SETTRANSFORM,
                                  sizeof( SVGA3dCmdSetTransform ) );
    Command->cid = ContextId;
    Command->type = Type;
    RtlCopyMemory( &Command->matrix, Transform, sizeof( float ) * 16 );
    DdFifoCommitAll( Svga );
}

VOID
Dd3dSetMaterial(
    _In_ PSVGA_DEVICE    Svga,
    _In_ ULONG32         ContextId,
    _In_ SVGA3dFace      Face,
    _In_ SVGA3dMaterial* Material
)
{
    SVGA3dCmdSetMaterial* Command;

    Command = Dd3dFifoReserveCmd( Svga,
                                  SVGA_3D_CMD_SETMATERIAL,
                                  sizeof( SVGA3dCmdSetMaterial ) );
    Command->cid = ContextId;
    Command->face = Face;
    RtlCopyMemory( &Command->material, Material, sizeof( SVGA3dMaterial ) );
    DdFifoCommitAll( Svga );
}

VOID
Dd3dSetLightEnabled(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      ContextId,
    _In_ ULONG32      Index,
    _In_ BOOLEAN      Enabled
)
{
    SVGA3dCmdSetLightEnabled* Command;

    Command = Dd3dFifoReserveCmd( Svga,
                                  SVGA_3D_CMD_SETLIGHTENABLED,
                                  sizeof( SVGA3dCmdSetLightEnabled ) );
    Command->cid = ContextId;
    Command->index = Index;
    Command->enabled = Enabled;
    DdFifoCommitAll( Svga );
}

VOID
Dd3dSetLightData(
    _In_ PSVGA_DEVICE     Svga,
    _In_ ULONG32          ContextId,
    _In_ ULONG32          Index,
    _In_ SVGA3dLightData* Data
)
{
    SVGA3dCmdSetLightData* Command;

    Command = Dd3dFifoReserveCmd( Svga,
                                  SVGA_3D_CMD_SETLIGHTENABLED,
                                  sizeof( SVGA3dCmdSetLightData ) );
    Command->cid = ContextId;
    Command->index = Index;
    RtlCopyMemory( &Command->data, Data, sizeof( SVGA3dLightData ) );
    DdFifoCommitAll( Svga );
}

VOID
Dd3dDefineShader(
    _In_ PSVGA_DEVICE     Svga,
    _In_ ULONG32          ContextId,
    _In_ ULONG32          ShaderId,
    _In_ SVGA3dShaderType Type,
    _In_ ULONG32*         ByteCode,
    _In_ ULONG32          ByteCodeLength
)
{
    SVGA3dCmdDefineShader* Command;

    if ( ( ByteCodeLength & 3 ) != 0 ) {
        // alignment?
        return;
    }

    Command = Dd3dFifoReserveCmd( Svga,
                                  SVGA_3D_CMD_SHADER_DEFINE,
                                  sizeof( SVGA3dCmdDefineShader ) + ByteCodeLength );
    Command->cid = ContextId;
    Command->shid = ShaderId;
    Command->type = Type;
    RtlCopyMemory( Command + 1, ByteCode, ByteCodeLength );
    DdFifoCommitAll( Svga );
}

VOID
Dd3dDestroyShader(
    _In_ PSVGA_DEVICE     Svga,
    _In_ ULONG32          ContextId,
    _In_ ULONG32          ShaderId,
    _In_ SVGA3dShaderType Type
)
{
    SVGA3dCmdDestroyShader* Command;

    Command = Dd3dFifoReserveCmd( Svga,
                                  SVGA_3D_CMD_SHADER_DESTROY,
                                  sizeof( SVGA3dCmdDestroyShader ) );
    Command->cid = ContextId;
    Command->shid = ShaderId;
    Command->type = Type;
    DdFifoCommitAll( Svga );
}

VOID
Dd3dSetShaderConst(
    _In_ PSVGA_DEVICE          Svga,
    _In_ ULONG32               ContextId,
    _In_ ULONG32               Register,
    _In_ SVGA3dShaderType      Type,
    _In_ SVGA3dShaderConstType ConstType,
    _In_ PVOID                 Value
)
{
    SVGA3dCmdSetShaderConst* Command;

    Command = Dd3dFifoReserveCmd( Svga,
                                  SVGA_3D_CMD_SET_SHADER_CONST,
                                  sizeof( SVGA3dCmdSetShaderConst ) );
    Command->cid = ContextId;
    Command->reg = Register;
    Command->type = Type;
    Command->ctype = ConstType;

    switch ( ConstType ) {
    case SVGA3D_CONST_TYPE_FLOAT:
    case SVGA3D_CONST_TYPE_INT:
        RtlCopyMemory( &Command->values, Value, sizeof( Command->values ) );
        break;
    case SVGA3D_CONST_TYPE_BOOL:
        RtlZeroMemory( &Command->values, sizeof( Command->values ) );
        Command->values[ 0 ] = *( ULONG32* )Value;
        break;
    default:
        break;
    }
    DdFifoCommitAll( Svga );
}

VOID
Dd3dBeginSetShader(
    _In_ PSVGA_DEVICE     Svga,
    _In_ ULONG32          ContextId,
    _In_ ULONG32          ShaderId,
    _In_ SVGA3dShaderType Type
)
{
    SVGA3dCmdSetShader* Command;

    Command = Dd3dFifoReserveCmd( Svga,
                                  SVGA_3D_CMD_SET_SHADER,
                                  sizeof( SVGA3dCmdSetShader ) );
    Command->cid = ContextId;
    Command->shid = ShaderId;
    Command->type = Type;
    DdFifoCommitAll( Svga );
}

VOID
Dd3dBeginPresent(
    _In_  PSVGA_DEVICE     Svga,
    _In_  ULONG32          SurfaceId,
    _Out_ SVGA3dCopyRect** Rectangles,
    _In_  ULONG32          RectangleCount
)
{
    SVGA3dCmdPresent* Command;

    Command = Dd3dFifoReserveCmd( Svga,
                                  SVGA_3D_CMD_PRESENT,
                                  sizeof( SVGA3dCmdPresent ) +
                                  sizeof( SVGA3dCopyRect ) * RectangleCount );
    Command->sid = SurfaceId;
    *Rectangles = ( SVGA3dCopyRect* )( Command + 1 );
    //DdFifoCommitAll( Svga );
}

VOID
Dd3dBeginClear(
    _In_  PSVGA_DEVICE     Svga,
    _In_  ULONG32          ContextId,
    _In_  SVGA3dClearFlag  Flags,
    _In_  ULONG32          Colour,
    _In_  float            Depth,
    _In_  ULONG32          Stencil,
    _Out_ SVGA3dRect**     Rectangles,
    _In_  ULONG32          RectangleCount
)
{
    SVGA3dCmdClear* Command;

    Command = Dd3dFifoReserveCmd( Svga,
                                  SVGA_3D_CMD_CLEAR,
                                  sizeof( SVGA3dCmdClear ) +
                                  sizeof( SVGA3dRect ) * RectangleCount );
    Command->cid = ContextId;
    Command->clearFlag = Flags;
    Command->color = Colour;
    Command->depth = Depth;
    Command->stencil = Stencil;
    *Rectangles = ( SVGA3dRect* )( Command + 1 );
    //DdFifoCommitAll( Svga );
}

VOID
Dd3dBeginDrawPrimitives(
    _In_  PSVGA_DEVICE           Svga,
    _In_  ULONG32                ContextId,
    _Out_ SVGA3dVertexDecl**     Vertices,
    _In_  ULONG32                VertexCount,
    _Out_ SVGA3dPrimitiveRange** Ranges,
    _In_  ULONG32                RangeCount
)
{
    SVGA3dCmdDrawPrimitives* Command;
    SVGA3dVertexDecl* VertexArray;
    SVGA3dPrimitiveRange* RangeArray;
    ULONG32 VertexSize = sizeof( SVGA3dVertexDecl ) * VertexCount;
    ULONG32 RangeSize = sizeof( SVGA3dPrimitiveRange ) * RangeCount;

    Command = Dd3dFifoReserveCmd( Svga,
                                  SVGA_3D_CMD_DRAW_PRIMITIVES,
                                  sizeof( SVGA3dCmdDrawPrimitives ) + VertexSize + RangeSize );

    Command->cid = ContextId;
    Command->numVertexDecls = VertexCount;
    Command->numRanges = RangeCount;

    VertexArray = ( SVGA3dVertexDecl* )( Command + 1 );
    RangeArray = ( SVGA3dPrimitiveRange* )( VertexArray + VertexCount );

    RtlZeroMemory( VertexArray, VertexCount );
    RtlZeroMemory( RangeArray, RangeCount );

    *Vertices = VertexArray;
    *Ranges = RangeArray;
}

VOID
Dd3dBeginSurfaceCopy(
    _In_  PSVGA_DEVICE          Svga,
    _In_  SVGA3dSurfaceImageId* Source,
    _In_  SVGA3dSurfaceImageId* Destination,
    _Out_ SVGA3dCopyBox**       Boxes,
    _In_  ULONG32               BoxCount
)
{
    SVGA3dCmdSurfaceCopy* Command;

    Command = Dd3dFifoReserveCmd( Svga,
                                  SVGA_3D_CMD_SURFACE_COPY,
                                  sizeof( SVGA3dCmdSurfaceCopy ) +
                                  sizeof( SVGA3dCopyBox ) * BoxCount );
    Command->src = *Source;
    Command->dest = *Destination;
    *Boxes = ( SVGA3dCopyBox* )( Command + 1 );
    RtlZeroMemory( *Boxes, sizeof( SVGA3dCopyBox ) * BoxCount );
    //DdFifoCommitAll( Svga );
}

VOID
Dd3dBeginSurfaceStretchBlt(
    _In_ PSVGA_DEVICE          Svga,
    _In_ SVGA3dSurfaceImageId* Source,
    _In_ SVGA3dSurfaceImageId* Destination,
    _In_ SVGA3dBox*            BoxSource,
    _In_ SVGA3dBox*            BoxDestination,
    _In_ SVGA3dStretchBltMode  Mode
)
{
    SVGA3dCmdSurfaceStretchBlt* Command;

    Command = Dd3dFifoReserveCmd( Svga,
                                  SVGA_3D_CMD_SURFACE_STRETCHBLT,
                                  sizeof( SVGA3dCmdSurfaceStretchBlt ) );
    Command->src = *Source;
    Command->dest = *Destination;
    Command->boxSrc = *BoxSource;
    Command->boxDest = *BoxDestination;
    Command->mode = Mode;
    DdFifoCommitAll( Svga );
}

VOID
Dd3dBeginSetViewport(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      ContextId,
    _In_ SVGA3dRect*  Rectangle
)
{
    SVGA3dCmdSetViewport* Command;

    Command = Dd3dFifoReserveCmd( Svga,
                                  SVGA_3D_CMD_SETVIEWPORT,
                                  sizeof( SVGA3dCmdSetViewport ) );
    Command->cid = ContextId;
    Command->rect = *Rectangle;
    DdFifoCommitAll( Svga );
}

VOID
Dd3dSetZRange(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      ContextId,
    _In_ float        Minimum,
    _In_ float        Maximum
)
{
    SVGA3dCmdSetZRange* Command;

    Command = Dd3dFifoReserveCmd( Svga,
                                  SVGA_3D_CMD_SETZRANGE,
                                  sizeof( SVGA3dCmdSetZRange ) );
    Command->cid = ContextId;
    Command->zRange.min = Minimum;
    Command->zRange.max = Maximum;
    DdFifoCommitAll( Svga );
}

VOID
Dd3dBeginSetTextureState(
    _In_  PSVGA_DEVICE         Svga,
    _In_  ULONG32              ContextId,
    _Out_ SVGA3dTextureState** States,
    _In_  ULONG32              StateCount
)
{
    SVGA3dCmdSetTextureState* Command;

    Command = Dd3dFifoReserveCmd( Svga,
                                  SVGA_3D_CMD_SETTEXTURESTATE,
                                  sizeof( SVGA3dCmdSetTextureState ) +
                                  sizeof( SVGA3dTextureState ) * StateCount );
    Command->cid = ContextId;
    *States = ( SVGA3dTextureState* )( Command + 1 );
}

VOID
Dd3dBeginSetRenderState(
    _In_  PSVGA_DEVICE        Svga,
    _In_  ULONG32             ContextId,
    _Out_ SVGA3dRenderState** States,
    _In_  ULONG32             StateCount
)
{
    SVGA3dCmdSetRenderState* Command;

    Command = Dd3dFifoReserveCmd( Svga,
                                  SVGA_3D_CMD_SETRENDERSTATE,
                                  sizeof( SVGA3dCmdSetRenderState ) +
                                  sizeof( SVGA3dRenderState ) * StateCount );
    Command->cid = ContextId;
    *States = ( SVGA3dRenderState* )( Command + 1 );
}

VOID
Dd3dBeginPresentReadback(
    _In_  PSVGA_DEVICE Svga,
    _Out_ SVGA3dRect** Rectangles,
    _In_  ULONG32      RectangleCount
)
{
    SVGA3dCmdSetRenderState* Command;

    Command = Dd3dFifoReserveCmd( Svga,
                                  SVGA_3D_CMD_PRESENT_READBACK,
                                  sizeof( SVGA3dRect ) * RectangleCount );
    *Rectangles = ( SVGA3dRect* )( Command );
}

VOID
Dd3dBeginBlitSurfaceToScreen(
    _In_     PSVGA_DEVICE          Svga,
    _In_     SVGA3dSurfaceImageId* SourceImage,
    _In_     SVGASignedRect*       SourceRectangle,
    _In_opt_ ULONG32               DestinationScreenId,
    _In_     SVGASignedRect*       DestinationRectangle,
    _In_opt_ SVGASignedRect**      ClipRectangles,
    _In_opt_ ULONG32               ClipRectangleCount
)
{
    SVGA3dCmdBlitSurfaceToScreen* Command;

    Command = Dd3dFifoReserveCmd( Svga,
                                  SVGA_3D_CMD_BLIT_SURFACE_TO_SCREEN,
                                  sizeof( SVGA3dCmdBlitSurfaceToScreen ) +
                                  sizeof( SVGASignedRect ) * ClipRectangleCount );
    Command->srcImage = *SourceImage;
    Command->srcRect = *SourceRectangle;
    Command->destScreenId = DestinationScreenId;
    Command->destRect = *DestinationRectangle;

    if ( ClipRectangles != NULL ) {

        *ClipRectangles = ( SVGASignedRect* )( Command + 1 );
    }
}

VOID
Dd3dBlitSurfaceToScreen(
    _In_     PSVGA_DEVICE          Svga,
    _In_     SVGA3dSurfaceImageId* SourceImage,
    _In_     SVGASignedRect*       SourceRectangle,
    _In_opt_ ULONG32               DestinationScreenId,
    _In_     SVGASignedRect*       DestinationRectangle
)
{
    Dd3dBeginBlitSurfaceToScreen( Svga,
                                  SourceImage,
                                  SourceRectangle,
                                  DestinationScreenId,
                                  DestinationRectangle,
                                  NULL, 0 );
    DdFifoCommitAll( Svga );
}
