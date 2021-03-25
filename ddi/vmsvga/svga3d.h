


#pragma once

#include "svga3d_reg.h"

VOID
Dd3dInitialize(
    _In_ PSVGA_DEVICE Svga
);

PVOID
Dd3dFifoReserveCmd(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      Type,
    _In_ ULONG32      ByteCount
);

VOID
Dd3dBeginDefineSurface(
    _In_  PSVGA_DEVICE        Svga,
    _In_  ULONG32             Id,
    _In_  SVGA3dSurfaceFlags  Flags,
    _In_  SVGA3dSurfaceFormat Format,
    _Out_ SVGA3dSurfaceFace** Faces,
    _Out_ SVGA3dSize**        Sizes,
    _In_  ULONG32             SizesCount
);

VOID
Dd3dDestroySurface(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      Id
);

VOID
Dd3dBeginSurfaceDma(
    _In_  PSVGA_DEVICE          Svga,
    _In_  SVGA3dGuestImage*     GuestImage,
    _In_  SVGA3dSurfaceImageId* HostImage,
    _In_  SVGA3dTransferType    Transfer,
    _Out_ SVGA3dCopyBox**       Boxes,
    _In_  ULONG32               BoxCount
);

VOID
Dd3dDefineContext(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      Id
);

VOID
Dd3dDestroyContext(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      ContextId
);

VOID
Dd3dSetRenderTarget(
    _In_ PSVGA_DEVICE           Svga,
    _In_ ULONG32                ContextId,
    _In_ SVGA3dRenderTargetType Type,
    _In_ SVGA3dSurfaceImageId*  Target
);

VOID
Dd3dSetTransform(
    _In_ PSVGA_DEVICE           Svga,
    _In_ ULONG32                ContextId,
    _In_ SVGA3dRenderTargetType Type,
    _In_ float*                 Transform
);

VOID
Dd3dSetMaterial(
    _In_ PSVGA_DEVICE    Svga,
    _In_ ULONG32         ContextId,
    _In_ SVGA3dFace      Face,
    _In_ SVGA3dMaterial* Material
);

VOID
Dd3dSetLightEnabled(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      ContextId,
    _In_ ULONG32      Index,
    _In_ BOOLEAN      Enabled
);

VOID
Dd3dSetLightData(
    _In_ PSVGA_DEVICE     Svga,
    _In_ ULONG32          ContextId,
    _In_ ULONG32          Index,
    _In_ SVGA3dLightData* Data
);

VOID
Dd3dDefineShader(
    _In_ PSVGA_DEVICE     Svga,
    _In_ ULONG32          ContextId,
    _In_ ULONG32          ShaderId,
    _In_ SVGA3dShaderType Type,
    _In_ ULONG32*         ByteCode,
    _In_ ULONG32          ByteCodeLength
);

VOID
Dd3dDestroyShader(
    _In_ PSVGA_DEVICE     Svga,
    _In_ ULONG32          ContextId,
    _In_ ULONG32          ShaderId,
    _In_ SVGA3dShaderType Type
);

VOID
Dd3dSetShaderConst(
    _In_ PSVGA_DEVICE          Svga,
    _In_ ULONG32               ContextId,
    _In_ ULONG32               Register,
    _In_ SVGA3dShaderType      Type,
    _In_ SVGA3dShaderConstType ConstType,
    _In_ PVOID                 Value
);

VOID
Dd3dBeginSetShader(
    _In_ PSVGA_DEVICE     Svga,
    _In_ ULONG32          ContextId,
    _In_ ULONG32          ShaderId,
    _In_ SVGA3dShaderType Type
);

VOID
Dd3dBeginPresent(
    _In_  PSVGA_DEVICE     Svga,
    _In_  ULONG32          SurfaceId,
    _Out_ SVGA3dCopyRect** Rectangles,
    _In_  ULONG32          RectangleCount
);

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
);

VOID
Dd3dBeginDrawPrimitives(
    _In_  PSVGA_DEVICE           Svga,
    _In_  ULONG32                ContextId,
    _Out_ SVGA3dVertexDecl**     Vertices,
    _In_  ULONG32                VertexCount,
    _Out_ SVGA3dPrimitiveRange** Ranges,
    _In_  ULONG32                RangeCount
);

VOID
Dd3dBeginSurfaceCopy(
    _In_  PSVGA_DEVICE          Svga,
    _In_  SVGA3dSurfaceImageId* Source,
    _In_  SVGA3dSurfaceImageId* Destination,
    _Out_ SVGA3dCopyBox**       Boxes,
    _In_  ULONG32               BoxCount
);

VOID
Dd3dBeginSurfaceStretchBlt(
    _In_ PSVGA_DEVICE          Svga,
    _In_ SVGA3dSurfaceImageId* Source,
    _In_ SVGA3dSurfaceImageId* Destination,
    _In_ SVGA3dBox*            BoxSource,
    _In_ SVGA3dBox*            BoxDestination,
    _In_ SVGA3dStretchBltMode  Mode
);

VOID
Dd3dBeginSetViewport(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      ContextId,
    _In_ SVGA3dRect*  Rectangle
);

VOID
Dd3dSetZRange(
    _In_ PSVGA_DEVICE Svga,
    _In_ ULONG32      ContextId,
    _In_ float        Minimum,
    _In_ float        Maximum
);

VOID
Dd3dBeginSetTextureState(
    _In_  PSVGA_DEVICE         Svga,
    _In_  ULONG32              ContextId,
    _Out_ SVGA3dTextureState** States,
    _In_  ULONG32              StateCount
);

VOID
Dd3dBeginSetRenderState(
    _In_  PSVGA_DEVICE        Svga,
    _In_  ULONG32             ContextId,
    _Out_ SVGA3dRenderState** States,
    _In_  ULONG32             StateCount
);

VOID
Dd3dBeginPresentReadback(
    _In_  PSVGA_DEVICE Svga,
    _Out_ SVGA3dRect** Rectangles,
    _In_  ULONG32      RectangleCount
);

VOID
Dd3dBeginBlitSurfaceToScreen(
    _In_     PSVGA_DEVICE          Svga,
    _In_     SVGA3dSurfaceImageId* SourceImage,
    _In_     SVGASignedRect*       SourceRectangle,
    _In_opt_ ULONG32               DestinationScreenId,
    _In_     SVGASignedRect*       DestinationRectangle,
    _In_opt_ SVGASignedRect**      ClipRectangles,
    _In_opt_ ULONG32               ClipRectangleCount
);

VOID
Dd3dBlitSurfaceToScreen(
    _In_     PSVGA_DEVICE          Svga,
    _In_     SVGA3dSurfaceImageId* SourceImage,
    _In_     SVGASignedRect*       SourceRectangle,
    _In_opt_ ULONG32               DestinationScreenId,
    _In_     SVGASignedRect*       DestinationRectangle
);
