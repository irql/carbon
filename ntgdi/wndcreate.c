

#include "driver.h"
#include "wnd.h"

KLOCKED_LIST g_ClassList = { 0 };
POBJECT_TYPE_DESCRIPTOR ObjectTypeWindow;
PKWINDOW g_RootWindow;

PWNDCLASSEX
NtGdiFindClassByName(
	__in PUNICODE_STRING ClassName
)
{

	KeAcquireSpinLock( &g_ClassList.Lock );

	PLIST_ENTRY Flink = g_ClassList.List;
	do {
		PWNDCLASSEX_INTERNAL Class = CONTAINING_RECORD( Flink, WNDCLASSEX_INTERNAL, ClassLinks );

		if ( RtlUnicodeStringCompare( ClassName, &Class->ClassName ) == 0 ) {

			KeReleaseSpinLock( &g_ClassList.Lock );
			return ( PWNDCLASSEX )Class;
		}

		Flink = Flink->Flink;
	} while ( Flink != g_ClassList.List );

	KeReleaseSpinLock( &g_ClassList.Lock );

	return NULL;
}

NTSTATUS
NtGdiRegisterClass(
	__in PWNDCLASSEX Class
)
{
	PWNDCLASSEX_INTERNAL InternalClass = ExAllocatePoolWithTag( sizeof( WNDCLASSEX_INTERNAL ), 'XECW' );

	_memcpy( ( void* )InternalClass, ( void* )Class, sizeof( WNDCLASSEX ) );

	if ( InternalClass->ClassInit == NULL ) {

		InternalClass->ClassInit = NtGdiDiWindowBaseClassInit;
	}

	KeAcquireSpinLock( &g_ClassList.Lock );
	if ( g_ClassList.List == NULL ) {

		g_ClassList.List = &InternalClass->ClassLinks;
		KeInitializeListHead( g_ClassList.List );
	}
	else {

		KeInsertListEntry( g_ClassList.List, &InternalClass->ClassLinks );
	}
	KeReleaseSpinLock( &g_ClassList.Lock );

	return STATUS_SUCCESS;
}

NTSTATUS
NtGdiUnregisterClass(
	__in PWNDCLASSEX Class
)
{
	Class;

	return STATUS_SUCCESS;
}



NTSTATUS
NtGdiCreateWindow(
	__in PUNICODE_STRING Name,
	__in PUNICODE_STRING ClassName,
	__in ULONG32 Flags,
	__in ULONG32 x,
	__in ULONG32 y,
	__in ULONG32 Width,
	__in ULONG32 Height,
	__in ULONG32 MenuId,
	__in HANDLE ParentHandle,
	__out PHANDLE Handle
)
{
	MenuId;
	Flags;

	STATIC OBJECT_ATTRIBUTES DefaultAttributes = { OBJ_PERMANENT, NULL };

	NTSTATUS ntStatus;
	PWNDCLASSEX Class = NtGdiFindClassByName( ClassName );

	if ( Class == NULL ) {

		return STATUS_UNSUCCESSFUL;
	}

	PKWINDOW Parent = NULL;

	if ( ParentHandle != 0 ) {

		ntStatus = ObReferenceObjectByHandle( ParentHandle, &Parent );

		if ( !NT_SUCCESS( ntStatus ) ) {

			return ntStatus;
		}

		POBJECT_TYPE_DESCRIPTOR Type;
		ntStatus = ObQueryObjectType( Parent, &Type );

		if ( !NT_SUCCESS( ntStatus ) ) {

			return ntStatus;
		}

		if ( Type != ObjectTypeWindow ) {

			return STATUS_INVALID_HANDLE;
		}

	}

	if ( g_RootWindow != NULL ) {

		y += WND_TITLE_BAR_H;
	}

	PKWINDOW Child;
	ntStatus = ObpCreateObject( &Child, &DefaultAttributes, ObjectTypeWindow );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	Child->MenuId = MenuId;

	Child->Rect.top = y;
	Child->Rect.bottom = y + Height;
	Child->Rect.left = x;
	Child->Rect.right = x + Width;

	_memcpy( ( void* )&Child->Name, ( void* )Name, sizeof( UNICODE_STRING ) );

	Child->Class = Class;
	Class->ClassInit( Child, Class );

	Child->PrimaryBuffer = MmAllocateMemory( Width * Height * 4, PAGE_READ | PAGE_WRITE );

	if ( Parent != NULL ) {

		Child->Parent = Parent;

		PKWINDOW Youngest = Parent;

		while ( Youngest->Child != NULL ) {

			Youngest = Youngest->Child;
		}

		Youngest->Child = Child;

	}
	else {

		Child->Parent = Child;

		if ( g_RootWindow == NULL ) {

			g_RootWindow = Child;
		}
		else {

			KeAcquireSpinLock( &g_WindowListLock );
			NtGdiWindowInsert( Child );
			KeReleaseSpinLock( &g_WindowListLock );
		}

		g_Focus = Child;
	}

	ntStatus = ObCreateHandle( Handle, Child );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	return STATUS_SUCCESS;
}

NTSTATUS
NtGdiWindowsInitializeSubsystem(

)
{
	NTSTATUS ntStatus;

	ntStatus = ObInitializeObjectType( L"WindowObject", sizeof( KWINDOW ), ' dnW', &ObjectTypeWindow );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	UNICODE_STRING ButtonClassName = RTL_CONSTANT_UNICODE_STRING( L"LIME_S" );

	WNDCLASSEX ButtonClass;
	ButtonClass.ClassInit = NtGdiDiButtonClassInit;
	ButtonClass.WndProc = NtGdiDiButtonWndProc;
	ButtonClass.Fill = 0xFFE1E1E1;
	ButtonClass.Border = 0xFFADADAD;
	ButtonClass.ClassName = ButtonClassName;

	ntStatus = NtGdiRegisterClass( &ButtonClass );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	UNICODE_STRING RootClassName = RTL_CONSTANT_UNICODE_STRING( L"rootclass" );

	WNDCLASSEX RootClass;
	RootClass.ClassName = RootClassName;
	RootClass.WndProc = NtGdiWndProcDefault;
	RootClass.Border = 0xFFFFFFFF;
	RootClass.Fill = 0xFFFFFFFF;//0xFF000000;
	RootClass.Flags = 0;
	RootClass.ClassInit = NULL;

	ntStatus = NtGdiRegisterClass( &RootClass );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	UNICODE_STRING RootWindowName = RTL_CONSTANT_UNICODE_STRING( L"root" );

	HANDLE RootHandle;

	ntStatus = NtGdiCreateWindow( &RootWindowName, &RootClassName, 0, 0, 0, g_Basic.Width, g_Basic.Height, 0, 0, &RootHandle );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	return STATUS_SUCCESS;
}