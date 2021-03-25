


#define DXINTERNAL
#include <carbusr.h>
#include "d3d.h"

#define DXGI_DDI_DEFINE_CONTROL( api, data, control_code ) \
DLLEXPORT \
NTSTATUS \
api( \
    _In_ HANDLE DriverHandle, \
    _In_ data Data \
) \
{ \
    IO_STATUS_BLOCK StatusBlock; \
    NtDeviceIoControlFile( DriverHandle, \
                           0, \
                           &StatusBlock, \
                           control_code, \
                           Data, \
                           sizeof( data ), \
                           Data, \
                           sizeof( data ) ); \
    return StatusBlock.Status; \
}

DXGI_DDI_DEFINE_CONTROL( D3dGetCaps, PD3DHAL_DATA_GET_CAPS, IOCTL_GET_CAPS );
DXGI_DDI_DEFINE_CONTROL( D3dSurfaceCreate, PD3DHAL_DATA_SURFACE_CREATE, IOCTL_SURFACE_CREATE );
DXGI_DDI_DEFINE_CONTROL( D3dSurfaceDestroy, PD3DHAL_DATA_SURFACE_DESTROY, IOCTL_SURFACE_DESTROY );
DXGI_DDI_DEFINE_CONTROL( D3dContextCreate, PD3DHAL_DATA_CONTEXT_CREATE, IOCTL_CONTEXT_CREATE );
DXGI_DDI_DEFINE_CONTROL( D3dContextDestroy, PD3DHAL_DATA_CONTEXT_DESTROY, IOCTL_CONTEXT_DESTROY );
DXGI_DDI_DEFINE_CONTROL( D3dSetRenderTarget, PD3DHAL_DATA_SET_RENDER_TARGET, IOCTL_SET_RENDER_TARGET );
DXGI_DDI_DEFINE_CONTROL( D3dSetRenderState, PD3DHAL_DATA_SET_RENDER_STATE, IOCTL_SET_RENDER_STATE );
DXGI_DDI_DEFINE_CONTROL( D3dSetTextureState, PD3DHAL_DATA_SET_TEXTURE_STATE, IOCTL_SET_TEXTURE_STATE );
DXGI_DDI_DEFINE_CONTROL( D3dSetTransform, PD3DHAL_DATA_SET_TRANSFORM, IOCTL_SET_TRANSFORM );
DXGI_DDI_DEFINE_CONTROL( D3dSetMaterial, PD3DHAL_DATA_SET_MATERIAL, IOCTL_SET_MATERIAL );
DXGI_DDI_DEFINE_CONTROL( D3dSetLightData, PD3DHAL_DATA_SET_LIGHT_DATA, IOCTL_SET_LIGHT_DATA );
DXGI_DDI_DEFINE_CONTROL( D3dSetLightEnabled, PD3DHAL_DATA_SET_LIGHT_ENABLED, IOCTL_SET_LIGHT_ENABLED );
DXGI_DDI_DEFINE_CONTROL( D3dShaderCreate, PD3DHAL_DATA_SHADER_CREATE, IOCTL_SHADER_CREATE );
DXGI_DDI_DEFINE_CONTROL( D3dShaderDestroy, PD3DHAL_DATA_SHADER_DESTROY, IOCTL_SHADER_DESTROY );
DXGI_DDI_DEFINE_CONTROL( D3dPresent, PD3DHAL_DATA_PRESENT, IOCTL_PRESENT );
DXGI_DDI_DEFINE_CONTROL( D3dClear, PD3DHAL_DATA_CLEAR, IOCTL_CLEAR );
DXGI_DDI_DEFINE_CONTROL( D3dBlt, PD3DHAL_DATA_BLT, IOCTL_BLT );
DXGI_DDI_DEFINE_CONTROL( D3dStretchBlt, PD3DHAL_DATA_STRETCH_BLT, IOCTL_STRETCH_BLT );
DXGI_DDI_DEFINE_CONTROL( D3dSetZRange, PD3DHAL_DATA_SET_ZRANGE, IOCTL_SET_ZRANGE );
DXGI_DDI_DEFINE_CONTROL( D3dSetViewport, PD3DHAL_DATA_SET_VIEWPORT, IOCTL_SET_VIEWPORT );

DLLEXPORT
NTSTATUS
D3dSubmitCommand(
    _In_ HANDLE DriverHandle
)
{
    IO_STATUS_BLOCK StatusBlock;
    NtDeviceIoControlFile( DriverHandle,
                           0,
                           &StatusBlock,
                           IOCTL_SUBMIT_COMMAND,
                           NULL,
                           0,
                           NULL,
                           0 );
    return StatusBlock.Status;
}

#undef DXGI_DDI_DEFINE_CONTROL

HANDLE
DdiOpenDeviceHandle(

)
{

    HANDLE DxgiHandle;
    IO_STATUS_BLOCK StatusBlock;
    OBJECT_ATTRIBUTES Attributes = { RTL_CONSTANT_STRING( L"\\Device\\DdAdapter0" ), { 0 }, 0 };

    NtCreateFile( &DxgiHandle,
                  &StatusBlock,
                  SYNCHRONIZE,
                  &Attributes,
                  0,
                  0,
                  0 );
    return DxgiHandle;
}

VOID
DdiDllLoad(
    _In_ PVOID ModuleBase,
    _In_ ULONG CallReason
)
{
    ModuleBase;
    CallReason;

    //
    // dxgi.sys should implement apis to create \device\display%d links, which can 
    // be enumerated and multi monitor support which be implemented at a driver level
    // for example where monitors are directly next to each other, display0 will be
    // at (0,0), size: (1920, 1080) and display1 is at (1920,0), size: (1920, 1080)
    // 
    //

    return;
}
