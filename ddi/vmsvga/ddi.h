


#pragma once

NTSTATUS
D3dSurfaceCreate(
    _In_ PDEVICE_OBJECT              Device,
    _In_ PD3DHAL_DATA_SURFACE_CREATE Data
);

NTSTATUS
D3dSetRenderTarget(
    _In_ PDEVICE_OBJECT                 Device,
    _In_ PD3DHAL_DATA_SET_RENDER_TARGET Data
);

NTSTATUS
D3dSetViewport(
    _In_ PDEVICE_OBJECT            Device,
    _In_ PD3DHAL_DATA_SET_VIEWPORT Data
);

NTSTATUS
D3dSetZRange(
    _In_ PDEVICE_OBJECT          Device,
    _In_ PD3DHAL_DATA_SET_ZRANGE Data
);

NTSTATUS
D3dSetRenderState(
    _In_ PDEVICE_OBJECT                Device,
    _In_ PD3DHAL_DATA_SET_RENDER_STATE Data
);

NTSTATUS
D3dClear(
    _In_ PDEVICE_OBJECT     Device,
    _In_ PD3DHAL_DATA_CLEAR Data
);

NTSTATUS
D3dContextCreate(
    _In_ PDEVICE_OBJECT              Device,
    _In_ PD3DHAL_DATA_CONTEXT_CREATE Data
);

NTSTATUS
D3dPresent(
    _In_ PDEVICE_OBJECT       Device,
    _In_ PD3DHAL_DATA_PRESENT Data
);

NTSTATUS
D3dSubmitCommand(
    _In_ PDEVICE_OBJECT Device
);
