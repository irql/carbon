


#include <carbsup.h>
#include "../dxgi/dxgi.h"
#include "../dxgi/ddi/ddi.h"

#define VESA_TAG 'ASEV'
#define VesaBuffer( _, x, y, w ) ( ( ULONG32* )( _ ) )[ (y) * (w) + (x) ] 

DLLIMPORT PULONG32 MappedFramebuffer;
DLLIMPORT ULONG32  DisplayWidth;
DLLIMPORT ULONG32  DisplayHeight;
DLLIMPORT ULONG32  BitsPerPixel;

ULONG32
NtDdiVesaAlphaBlend(
    _In_ ULONG32 Src,
    _In_ ULONG32 Dst
)
{
    //https://www.daniweb.com/programming/software-development/code/216791/alpha-blend-algorithm

    UCHAR Alpha = ( UCHAR )( Src >> 24 );

    if ( Alpha == 255 )
        return Src;

    if ( Alpha == 0 )
        return Dst;

    ULONG32 RedBlue = ( ( ( Src & 0x00ff00ff ) * Alpha ) + ( ( Dst & 0x00ff00ff ) * ( 0xff - Alpha ) ) ) & 0xff00ff00;
    ULONG32 Green = ( ( ( Src & 0x0000ff00 ) * Alpha ) + ( ( Dst & 0x0000ff00 ) * ( 0xff - Alpha ) ) ) & 0x00ff0000;

    return ( Src & 0xff000000 ) | ( ( RedBlue | Green ) >> 8 );
}

VOID
NtDdiVesaInitializeDC(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PDC            DeviceContext
)
{
    DeviceObject;

    STATIC BOOLEAN ScreenDC = FALSE;

    if ( !ScreenDC ) {

        DeviceContext->DeviceSpecific = MappedFramebuffer;
        DeviceContext->ClientArea.Left = 0;
        DeviceContext->ClientArea.Top = 0;
        DeviceContext->ClientArea.Bottom = DisplayHeight;
        DeviceContext->ClientArea.Right = DisplayWidth;
        ScreenDC = TRUE;
        return;
    }

    DeviceContext->DeviceSpecific = MmAllocatePoolWithTag(
        NonPagedPoolZeroed,
        ( DeviceContext->ClientArea.Bottom - DeviceContext->ClientArea.Top ) *
        ( DeviceContext->ClientArea.Right - DeviceContext->ClientArea.Left ) *
        4,
        VESA_TAG );
}

VOID
NtDdiVesaReleaseDC(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PDC            DeviceContext
)
{
    DeviceObject;
    MmFreePoolWithTag( DeviceContext->DeviceSpecific,
                       VESA_TAG );
}

VOID
NtDdiVesaBlt(
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

    ULONG32 pointx;
    ULONG32 pointy;

    pointx = 0;
    pointy = 0;

    for ( ; pointy < srch; ) {

        // SPEEEEEEEEEEEEEEEEED
        if (
            ( ( LONG32 )dstx + ( LONG32 )pointx ) < DestinationContext->ClientArea.Right - DestinationContext->ClientArea.Left &&
            ( ( LONG32 )dsty + ( LONG32 )pointy ) < DestinationContext->ClientArea.Bottom - DestinationContext->ClientArea.Top &&
            ( ( LONG32 )dstx + ( LONG32 )pointx ) >= 0 &&
            ( ( LONG32 )dsty + ( LONG32 )pointy ) >= 0 &&
            ( ( LONG32 )srcx + ( LONG32 )pointx ) < SourceContext->ClientArea.Right - SourceContext->ClientArea.Left &&
            ( ( LONG32 )srcy + ( LONG32 )pointy ) < SourceContext->ClientArea.Bottom - SourceContext->ClientArea.Top &&
            ( ( LONG32 )srcx + ( LONG32 )pointx ) >= 0 &&
            ( ( LONG32 )srcy + ( LONG32 )pointy ) >= 0 ) {
#if 0
            VesaBuffer( DestinationContext->DeviceSpecific,
                        dstx + pointx,
                        dsty + pointy,
                        DestinationContext->ClientArea.Right - DestinationContext->ClientArea.Left ) =
                VesaBuffer( SourceContext->DeviceSpecific,
                            srcx + pointx,
                            srcy + pointy,
                            SourceContext->ClientArea.Right - SourceContext->ClientArea.Left );//srcw );
#endif
            VesaBuffer( DestinationContext->DeviceSpecific,
                        dstx + pointx,
                        dsty + pointy,
                        DestinationContext->ClientArea.Right - DestinationContext->ClientArea.Left ) =
                NtDdiVesaAlphaBlend( VesaBuffer( SourceContext->DeviceSpecific,
                                                 srcx + pointx,
                                                 srcy + pointy,
                                                 SourceContext->ClientArea.Right - SourceContext->ClientArea.Left ),
                                     VesaBuffer( DestinationContext->DeviceSpecific,
                                                 dstx + pointx,
                                                 dsty + pointy,
                                                 DestinationContext->ClientArea.Right - DestinationContext->ClientArea.Left ) );
    }

        pointx++;

        if ( pointx >= srcw ) {

            pointx = 0;
            pointy++;
        }
}
}

VOID
NtDdiVesaBltBits(
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
    DeviceObject;

    ULONG32 pointx;
    ULONG32 pointy;

    pointx = 0;
    pointy = 0;

    for ( ; pointy < srch; ) {

        // SPEEEEEEEEEEEEEEEEED
        if (
            ( ( LONG32 )dstx + ( LONG32 )pointx ) <= context->ClientArea.Right - context->ClientArea.Left &&
            ( ( LONG32 )dsty + ( LONG32 )pointy ) <= context->ClientArea.Bottom - context->ClientArea.Top &&
            ( ( LONG32 )dstx + ( LONG32 )pointx ) >= 0 &&
            ( ( LONG32 )dsty + ( LONG32 )pointy ) >= 0 ) {
#if 0
            VesaBuffer( context->DeviceSpecific,
                        dstx + pointx,
                        dsty + pointy,
                        context->ClientArea.Right - context->ClientArea.Left ) =
                VesaBuffer( srcbits,
                            srcx + pointx,
                            srcy + pointy,
                            srcw );
#endif
            VesaBuffer( context->DeviceSpecific,
                        dstx + pointx,
                        dsty + pointy,
                        context->ClientArea.Right - context->ClientArea.Left ) =
                NtDdiVesaAlphaBlend( VesaBuffer( srcbits,
                                                 srcx + pointx,
                                                 srcy + pointy,
                                                 srcw ),
                                     VesaBuffer( context->DeviceSpecific,
                                                 dstx + pointx,
                                                 dsty + pointy,
                                                 context->ClientArea.Right - context->ClientArea.Left ) );

    }

        pointx++;

        if ( pointx >= srcw ) {

            pointx = 0;
            pointy++;
        }
}
}

VOID
NtDdiVesaSetPixel(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PDC            Context,
    _In_ ULONG32        x,
    _In_ ULONG32        y,
    _In_ ULONG32        Color
)
{
    DeviceObject;
    if (
        ( LONG32 )x <= Context->ClientArea.Right - Context->ClientArea.Left &&
        ( LONG32 )y <= Context->ClientArea.Bottom - Context->ClientArea.Top &&
        ( LONG32 )x >= 0 &&
        ( LONG32 )y >= 0 ) {
        VesaBuffer( Context->DeviceSpecific,
                    x,
                    y,
                    Context->ClientArea.Right - Context->ClientArea.Left ) = Color;
    }
}

VOID
NtDdiVesaClear(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PDC            Context,
    _In_ ULONG32        x,
    _In_ ULONG32        y,
    _In_ ULONG32        w,
    _In_ ULONG32        h,
    _In_ ULONG32        Color
)
{
    DeviceObject;

    ULONG32 pointx;
    ULONG32 pointy;

    pointx = 0;
    pointy = 0;

    for ( ; pointy < h; ) {

        // SPEEEEEEEEEEEEEEEEED
        if (
            ( ( LONG32 )x + ( LONG32 )pointx ) <= Context->ClientArea.Right - Context->ClientArea.Left &&
            ( ( LONG32 )y + ( LONG32 )pointy ) <= Context->ClientArea.Bottom - Context->ClientArea.Top &&
            ( ( LONG32 )x + ( LONG32 )pointx ) >= 0 &&
            ( ( LONG32 )y + ( LONG32 )pointy ) >= 0 ) {
            VesaBuffer( Context->DeviceSpecific,
                        x + pointx,
                        y + pointy,
                        Context->ClientArea.Right - Context->ClientArea.Left ) = Color;

        }

        pointx++;

        if ( pointx >= w ) {

            pointx = 0;
            pointy++;
        }
    }
}

VOID
NtDdiVesaGetMode(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ ULONG32*       Width,
    _In_ ULONG32*       Height,
    _In_ ULONG32*       BitDepth
)
{
    DeviceObject;
    *Width = DisplayWidth;
    *Height = DisplayHeight;
    *BitDepth = BitsPerPixel;
}

NTSTATUS
NtDdiVesaSetMode(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ ULONG32        Width,
    _In_ ULONG32        Height,
    _In_ ULONG32        BitDepth
)
{
    DeviceObject;
    Width;
    Height;
    BitDepth;
    return DXSTATUS_UNSUPPORTED;
}

NTSTATUS
DriverLoad(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    DriverObject;
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING( L"\\Device\\VESA0" );
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_OBJECT DdiDeviceObject;
    PD3DHAL D3dHal;

    //
    // implement v86 and setup vesa via vbe 
    //

    IoCreateDevice( DriverObject,
                    0,
                    &DeviceName,
                    0,
                    &DeviceObject );

    DdCreateAdapter( DeviceObject,
                     &DdiDeviceObject );

    D3dHal = DdGetAdapterD3dHal( DdiDeviceObject );
    D3dHal->D3dHalVersion = 0; // no 3d accel.
    D3dHal->NtDdiInitializeDC = NtDdiVesaInitializeDC;
    D3dHal->NtDdiReleaseDC = NtDdiVesaReleaseDC;
    D3dHal->NtDdiBltDC = NtDdiVesaBlt;
    D3dHal->NtDdiBltBitsDC = NtDdiVesaBltBits;
    D3dHal->NtDdiSetPixelDC = NtDdiVesaSetPixel;
    D3dHal->NtDdiClearDC = NtDdiVesaClear;

    D3dHal->NtDdiGetMode = NtDdiVesaGetMode;
    D3dHal->NtDdiSetMode = NtDdiVesaSetMode;

    DdiDeviceObject->DeviceCharacteristics &= ~DEV_INITIALIZING;

    return STATUS_SUCCESS;
}
