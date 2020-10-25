

#include "driver.h"
#include "wnd.h"

#include "svga.h"

KBASIC_INFO g_Basic;

NTSTATUS
DriverEntry(
	__in PDRIVER_OBJECT DriverObject
);

NTSTATUS
DriverUnload(
	__in PDRIVER_OBJECT DriverObject
);

NTSTATUS
DriverDispatch(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp
);

NTSTATUS
DriverEntry(
	__in PDRIVER_OBJECT DriverObject
)
{

	DriverObject->DriverUnload = DriverUnload;

	PGDI_INFO GdiInfo = VbeGetInfo( );

	SvDriverInitialize( );
	SvSetMode( 1280, 720, 32 );

	g_Basic.Width = 1280;
	g_Basic.Height = 720;
	g_Basic.Framebuffer = g_SVGA.FramebufferBase;

	GdiInfo->Width = 1280;
	GdiInfo->Height = 720;
	GdiInfo->Framebuffer = g_Basic.Framebuffer;

	g_Basic.Doublebuffer = ( ULONG32* )MmAllocateMemory( g_Basic.Height * g_Basic.Width * 4, PAGE_READ | PAGE_WRITE );

	SVGAFifoCmdDefineAlphaCursor Cursor = {
		.id = 0,
		.hotspotX = 0,
		.hotspotY = 0,
		.width = 32,
		.height = 32
	};

	SvBeginDefineAlphaCursor( &Cursor, &g_Cursor.Buffer );

	NtMouseInstall( );
	NtKeyboardInstall( );

	NtInitializeCursor( );

	SvFifoCommitAll( );
	SvMoveCursor( 1, 0, 0 );

	NtGdiWindowsInitializeSubsystem( );

	UNICODE_STRING RootClassName = RTL_CONSTANT_UNICODE_STRING( L"asswipe" );
	WNDCLASSEX RootClass;
	RootClass.ClassName = RootClassName;
	RootClass.WndProc = NtGdiWndProcDefault;
	RootClass.Border = 0xFF707070;
	RootClass.Fill = 0xFFF0F0F0;
	RootClass.Flags = 0;
	RootClass.ClassInit = NULL;
	NtGdiRegisterClass( &RootClass );

	UNICODE_STRING RootWindowName = RTL_CONSTANT_UNICODE_STRING( L"pog" );
	HANDLE AssHandle;

	NtGdiCreateWindow( &RootWindowName, &RootClassName, 0, 80, 140, 400, 200, 0, 0, &AssHandle );

	UNICODE_STRING ButtonText = RTL_CONSTANT_UNICODE_STRING( L"LIME_S" );
	HANDLE ButtonHandle;

	NtGdiCreateWindow( &ButtonText, &ButtonText, 0, 4, 7, 80, 23, 1001, AssHandle, &ButtonHandle );

	UNICODE_STRING RootClassName1 = RTL_CONSTANT_UNICODE_STRING( L"pogchamp" );
	WNDCLASSEX RootClass1;
	RootClass1.ClassName = RootClassName1;
	RootClass1.WndProc = NtGdiWndProcDefault;
	RootClass1.Border = 0xFF707070;
	RootClass1.Fill = 0xFFF0F0F0;
	RootClass1.Flags = 0;
	RootClass1.ClassInit = NULL;
	NtGdiRegisterClass( &RootClass1 );

	UNICODE_STRING RootWindowName1 = RTL_CONSTANT_UNICODE_STRING( L"What?" );
	HANDLE AssHandle1;

	NtGdiCreateWindow( &RootWindowName1, &RootClassName1, 0, 120, 140, 800, 500, 0, 0, &AssHandle1 );

	UNICODE_STRING ButtonText1 = RTL_CONSTANT_UNICODE_STRING( L"ANDER_S" );
	HANDLE ButtonHandle1;

	NtGdiCreateWindow( &ButtonText1, &ButtonText, 0, 14, 84, 160, 23, 1001, AssHandle1, &ButtonHandle1 );

#if 1
	HANDLE RenderThread;
	HANDLE SystemProcess = KeQueryCurrentProcess( );
	KeCreateThread( &RenderThread, SystemProcess, ( PKSTART_ROUTINE )NtSvRenderLoop, NULL, 0x2000, 0x2000 );

	ZwClose( RenderThread );
	ZwClose( SystemProcess );
#endif

	return STATUS_SUCCESS;
}

NTSTATUS
DriverUnload(
	__in PDRIVER_OBJECT DriverObject
)
{

	//
	//	this is called when the system crashes and any graphics cards should be disabled.
	//

	//g_Basic.Framebuffer[ 0 ] = 0xFFFF0000;

	SvUpdate( 0, 0, 1280, 720 );
	SvWriteRegister( SVGA_REG_ENABLE, FALSE );

	DriverObject;

	return STATUS_SUCCESS;
}
