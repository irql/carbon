


#define NTUSER_INTERNAL
#include <carbsup.h>
#include "usersup.h"
#include "ntuser.h"
#include "../../ddi/dxgi/ddi/ddi.h"

//
// (check comment in driver.c)
// dc's are more of a dxgi thing
//
// ouch, this is gonna require some sort of 
// re-designing of dxgi ddi
//

PDC NtScreenDC;

NTSTATUS
NtDdiCreateDC(
    _Out_ PDC*  DeviceContext,
    _In_  PRECT Rect
)
{
    NTSTATUS ntStatus;
    OBJECT_ATTRIBUTES Context = { 0 };
    UNICODE_STRING Adapter = RTL_CONSTANT_STRING( L"\\Device\\DdAdapter0" );

    //
    // no idea how im gonna do multi-monitor support and how the actual adapter 
    // will be selected but for now, we can just have one graphics driver which
    // creates one adapter, and that'd be DdAdapter0.
    //

    ntStatus = ObCreateObject( DeviceContext,
                               NtDeviceContext,
                               &Context,
                               sizeof( DC ) );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    ntStatus = ObReferenceObjectByName( &( *DeviceContext )->DeviceObject,
                                        &Adapter,
                                        NULL );
    if ( !NT_SUCCESS( ntStatus ) ) {

        ObDereferenceObject( *DeviceContext );
        return ntStatus;
    }

    ( *DeviceContext )->ClientArea = *Rect;

    DdGetAdapterD3dHal( ( *DeviceContext )->DeviceObject )->
        NtDdiInitializeDC( ( *DeviceContext )->DeviceObject, ( *DeviceContext ) );

    return STATUS_SUCCESS;
}

NTSTATUS
NtGetDC(
    _Out_ PHANDLE ContextHandle,
    _In_  HANDLE  WindowHandle
)
{
    NTSTATUS ntStatus;
    PKWND WindowObject;

    if ( WindowHandle == 0 ) {

        ntStatus = ObOpenObjectFromPointer( ContextHandle,
                                            NtScreenDC,
                                            0,
                                            0,
                                            UserMode );
        return ntStatus;
    }

    ntStatus = ObReferenceObjectByHandle( &WindowObject,
                                          WindowHandle,
                                          0,
                                          UserMode,
                                          NtWindowObject );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    ntStatus = ObOpenObjectFromPointer( ContextHandle,
                                        WindowObject->BackContext,
                                        0,
                                        0,
                                        UserMode );
    ObDereferenceObject( WindowObject );
    return ntStatus;
}

VOID
NtDdiBeginPaint(
    _Out_ PDC*  Context,
    _In_  PKWND Window
)
{

    *Context = Window->BackContext;
    Window->PaintBegan = TRUE;
}

VOID
NtDdiEndPaint(
    _In_ PKWND Window
)
{

    NtDdiBlt( Window->BackContext,
              0,
              0,
              Window->BackContext->ClientArea.Right -
              Window->BackContext->ClientArea.Left,
              Window->BackContext->ClientArea.Bottom -
              Window->BackContext->ClientArea.Top,
              Window->FrontContext,
              0,
              0 );
    ZwSignalEvent( UpdateEvent, TRUE );

    Window->PaintBegan = FALSE;
}

VOID
NtBeginPaint(
    _Out_ PHANDLE ContextHandle,
    _In_  HANDLE  WindowHandle
)
{
    NTSTATUS ntStatus;
    PKWND WindowObject;

    ntStatus = ObReferenceObjectByHandle( &WindowObject,
                                          WindowHandle,
                                          0,
                                          UserMode,
                                          NtWindowObject );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return;
    }

    ntStatus = ObOpenObjectFromPointer( ContextHandle,
                                        WindowObject->BackContext,
                                        0,
                                        0,
                                        UserMode );
    if ( !NT_SUCCESS( ntStatus ) ) {

        ObDereferenceObject( WindowObject );
        return;
    }

    WindowObject->PaintBegan = TRUE;

    ObDereferenceObject( WindowObject );
    return;
}

VOID
NtEndPaint(
    _In_ HANDLE WindowHandle
)
{
    NTSTATUS ntStatus;
    PKWND WindowObject;

    ntStatus = ObReferenceObjectByHandle( &WindowObject,
                                          WindowHandle,
                                          0,
                                          UserMode,
                                          NtWindowObject );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return;
    }

    NtDdiEndPaint( WindowObject );
    ObDereferenceObject( WindowObject );
    return;
}

VOID
NtDdiBlt(
    _In_ PDC     SourceContext,
    _In_ ULONG32 srcx,
    _In_ ULONG32 srcy,
    _In_ ULONG32 srcw,
    _In_ ULONG32 srch,
    _In_ PDC     DestinationContext,
    _In_ ULONG32 dstx,
    _In_ ULONG32 dsty
)
{

    DdGetAdapterD3dHal( SourceContext->DeviceObject )->NtDdiBltDC(
        SourceContext->DeviceObject,
        SourceContext,
        srcx,
        srcy,
        srcw,
        srch,
        DestinationContext,
        dstx,
        dsty );
}

VOID
NtBlt(
    _In_ HANDLE  SourceContext,
    _In_ ULONG32 srcx,
    _In_ ULONG32 srcy,
    _In_ ULONG32 srcw,
    _In_ ULONG32 srch,
    _In_ HANDLE  DestinationContext,
    _In_ ULONG32 dstx,
    _In_ ULONG32 dsty
)
{
    NTSTATUS ntStatus;
    PDC Source;
    PDC Destination;

    ntStatus = ObReferenceObjectByHandle( &Source,
                                          SourceContext,
                                          0,
                                          UserMode,
                                          NtDeviceContext );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return;
    }

    ntStatus = ObReferenceObjectByHandle( &Destination,
                                          DestinationContext,
                                          0,
                                          UserMode,
                                          NtDeviceContext );
    if ( !NT_SUCCESS( ntStatus ) ) {

        ObDereferenceObject( Source );
        return;
    }

    NtDdiBlt( Source, srcx, srcy, srcw, srch, Destination, dstx, dsty );

    ObDereferenceObject( Source );
    ObDereferenceObject( Destination );
}

VOID
NtDdiBltBits(
    _In_ PVOID   srcbits,
    _In_ ULONG32 srcx,
    _In_ ULONG32 srcy,
    _In_ ULONG32 srcw,
    _In_ ULONG32 srch,
    _In_ PDC     context,
    _In_ ULONG32 dstx,
    _In_ ULONG32 dsty
)
{
    DdGetAdapterD3dHal( context->DeviceObject )->NtDdiBltBitsDC(
        context->DeviceObject,
        srcbits,
        srcx,
        srcy,
        srcw,
        srch,
        context,
        dstx,
        dsty );
}

VOID
NtBltBits(
    _In_ PVOID   srcbits,
    _In_ ULONG32 srcx,
    _In_ ULONG32 srcy,
    _In_ ULONG32 srcw,
    _In_ ULONG32 srch,
    _In_ HANDLE  context,
    _In_ ULONG32 dstx,
    _In_ ULONG32 dsty
)
{
    NTSTATUS ntStatus;
    PDC dst;

    ntStatus = ObReferenceObjectByHandle( &dst,
                                          context,
                                          0,
                                          UserMode,
                                          NtDeviceContext );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return;
    }

    NtDdiBltBits( srcbits, srcx, srcy, srcw, srch, dst, dstx, dsty );
    ObDereferenceObject( dst );
}

VOID
NtDdiSetPixel(
    _In_ PDC     Context,
    _In_ ULONG32 x,
    _In_ ULONG32 y,
    _In_ ULONG32 Color
)
{
    DdGetAdapterD3dHal( Context->DeviceObject )->NtDdiSetPixelDC(
        Context->DeviceObject,
        Context,
        x,
        y,
        Color );
}

VOID
NtSetPixel(
    _In_ HANDLE  ContextHandle,
    _In_ ULONG32 x,
    _In_ ULONG32 y,
    _In_ ULONG32 Color
)
{
    NTSTATUS ntStatus;
    PDC Context;

    ntStatus = ObReferenceObjectByHandle( &Context,
                                          ContextHandle,
                                          0,
                                          UserMode,
                                          NtDeviceContext );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return;
    }

    NtDdiSetPixel( Context, x, y, Color );
    ObDereferenceObject( Context );
}

VOID
NtDdiClearDC(
    _In_ PDC     Context,
    _In_ ULONG32 x,
    _In_ ULONG32 y,
    _In_ ULONG32 w,
    _In_ ULONG32 h,
    _In_ ULONG32 Color
)
{
    DdGetAdapterD3dHal( Context->DeviceObject )->NtDdiClearDC(
        Context->DeviceObject,
        Context,
        x,
        y,
        w,
        h,
        Color );
}

VOID
NtClearDC(
    _In_ HANDLE  ContextHandle,
    _In_ ULONG32 x,
    _In_ ULONG32 y,
    _In_ ULONG32 w,
    _In_ ULONG32 h,
    _In_ ULONG32 Color
)
{
    NTSTATUS ntStatus;
    PDC Context;

    ntStatus = ObReferenceObjectByHandle( &Context,
                                          ContextHandle,
                                          0,
                                          UserMode,
                                          NtDeviceContext );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return;
    }

    NtDdiClearDC( Context, x, y, w, h, Color );
    ObDereferenceObject( Context );
}
