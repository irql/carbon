

#include "driver.h"
#include "wnd.h"

POBJECT_TYPE_DESCRIPTOR ObjectTypeConsole = 0;

VOID
NtGdiConsoleWndProc(
	__in PKWINDOW Window,
	__in ULONG32 Message,
	__in ULONG32 Param1,
	__in ULONG32 Param2
)
{
	Window;
	Message;
	Param1;
	Param2;

	switch ( Message ) {
	case WM_COMMAND: {
		PKCONSOLE Console = NtGdiReferenceConsoleFromWindow( Window );

		if ( Console->Flags & CON_FLAG_READING ) {

			if ( Window->Name.Buffer[ Window->Name.Length - 1 ] == '\n' ) {

				Console->Flags &= ~CON_FLAG_READING;
				Window->Flags |= EDITBOX_REJECT_INPUT;
			}
		}

		//Param2.

		break;
	}
	default:
		break;
	}

}

PKCONSOLE
NtGdiReferenceConsoleFromWindow(
	__in PKWINDOW Window
)
{
	KeAcquireSpinLock( &ObjectTypeConsole->ObjectList.Lock );

	PLIST_ENTRY Flink = ObjectTypeConsole->ObjectList.List;

	do {
		POBJECT_ENTRY_HEADER ConsoleHeader = CONTAINING_RECORD( Flink, OBJECT_ENTRY_HEADER, ObjectList );
		PKCONSOLE ConsoleObject = ( PKCONSOLE )( ConsoleHeader + 1 );

		if ( ConsoleObject->Console == Window ) {

			KeReleaseSpinLock( &ObjectTypeConsole->ObjectList.Lock );
			return ConsoleObject;
		}

		Flink = Flink->Flink;
	} while ( Flink != ObjectTypeConsole->ObjectList.List );

	KeReleaseSpinLock( &ObjectTypeConsole->ObjectList.Lock );

	return NULL;
}

NTSTATUS
NtGdiConsoleInitializeSubsystem(

)
{

	NTSTATUS ntStatus;

	ntStatus = ObInitializeObjectType( L"ConsoleObject", sizeof( KCONSOLE ), ' NOC', &ObjectTypeConsole );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	UNICODE_STRING ConsoleClassName = RTL_CONSTANT_UNICODE_STRING( L"ConClass" );
	WNDCLASSEX ConsoleClass;
	ConsoleClass.Border = 0;
	ConsoleClass.ClassInit = NULL;
	ConsoleClass.ClassName = ConsoleClassName;
	ConsoleClass.Fill = 0xFF0C0C0C;
	ConsoleClass.Border = 0xFF000000;
	ConsoleClass.WndProc = NtGdiConsoleWndProc;

	ntStatus = NtGdiRegisterClass( &ConsoleClass );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	return STATUS_SUCCESS;
}

NTSTATUS
NtGdiCreateConsole(
	__out PHANDLE ConsoleHandle,
	__in PUNICODE_STRING Name,
	__in ULONG32 Flags,
	__in ULONG32 x,
	__in ULONG32 y
)
{
	Flags;

	//KeProbeStringForRead( Name );
	//KeProbeForWrite( ConsoleHandle, sizeof( HANDLE ) );

	STATIC OBJECT_ATTRIBUTES DefaultAttributes = { OBJ_PERMANENT, NULL };
	STATIC UNICODE_STRING ConsoleClassName = RTL_CONSTANT_UNICODE_STRING( L"ConClass" );
	STATIC UNICODE_STRING EditboxClassName = RTL_CONSTANT_UNICODE_STRING( L"SIME_S" );

	NTSTATUS ntStatus;

	HANDLE ConsoleWindowHandle = 0;
	HANDLE ConsoleEditboxHandle = 0;

	PKWINDOW ConsoleWindow = NULL;
	PKCONSOLE ConsoleObject = NULL;
	ntStatus = ObpCreateObject( &ConsoleObject, &DefaultAttributes, ObjectTypeConsole );

	if ( !NT_SUCCESS( ntStatus ) ) {

		goto done;
	}

	PUNICODE_STRING KernelName;
	RtlAllocateAndInitUnicodeStringEx( &KernelName, Name->Buffer );

	ntStatus = NtGdiCreateWindow( KernelName, &ConsoleClassName, 0, x, y, CONSOLE_DEFAULT_WIDTH, CONSOLE_DEFAULT_HEIGHT, 0, 0, &ConsoleWindowHandle );

	if ( !NT_SUCCESS( ntStatus ) ) {

		goto done;
	}

	ntStatus = NtGdiCreateWindow( KernelName, &EditboxClassName, 0, 0, 0, CONSOLE_DEFAULT_WIDTH, CONSOLE_DEFAULT_HEIGHT, 0, ConsoleWindowHandle, &ConsoleEditboxHandle );

	if ( !NT_SUCCESS( ntStatus ) ) {

		goto done;
	}

	ntStatus = ObReferenceObjectByHandle( ConsoleWindowHandle, &ConsoleWindow );

	if ( !NT_SUCCESS( ntStatus ) ) {

		goto done;
	}

	ConsoleObject->Console = ConsoleWindow;

	( ( PDI_EDIT_BOX )ConsoleWindow->Child->DiFlink )->Fill = 0xFF0C0C0C;
	ConsoleWindow->Child->Flags |= EDITBOX_REJECT_INPUT;
	ConsoleWindow->Child->Flags |= EDITBOX_NO_FOCUS_OUTLINE;

	ntStatus = ObCreateHandle( ConsoleHandle, ConsoleObject );

	if ( !NT_SUCCESS( ntStatus ) ) {

		goto done;
	}

done:;

	if ( !NT_SUCCESS( ntStatus ) ) {

		if ( ConsoleWindow ) {

			//impl close window.
			//ObDestroyObject( ConsoleWindow );
		}

		if ( ConsoleObject ) {

			ObDestroyObject( ConsoleObject );
		}
	}

	if ( ConsoleEditboxHandle ) {

		ZwClose( ConsoleEditboxHandle );
	}

	if ( ConsoleWindowHandle ) {

		ZwClose( ConsoleWindowHandle );
	}

	return ntStatus;
}

NTSTATUS
NtGdiReadConsole(
	__in HANDLE ConsoleHandle,
	__in PWCHAR Buffer,
	__in ULONG32 Length
)
{
	Length;

	//vuln because of buffer.

	KeProbeForRead( Buffer, Length * sizeof( WCHAR ) );

	NTSTATUS ntStatus;
	PKCONSOLE Console;

	ntStatus = ObReferenceObjectByHandle( ConsoleHandle, &Console );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	POBJECT_TYPE_DESCRIPTOR Type;

	ntStatus = ObQueryObjectType( Console, &Type );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	if ( Type != ObjectTypeConsole ) {

		return STATUS_INVALID_HANDLE;
	}

	PKWINDOW EditboxWindow = Console->Console->Child;
	ULONG32 OriginalLength = EditboxWindow->Name.Length;

	EditboxWindow->Flags &= ~EDITBOX_REJECT_INPUT;
	Console->Flags |= CON_FLAG_READING;

	while ( Console->Flags & CON_FLAG_READING ) {

		__halt( );
	}

	lstrcpyW( Buffer, &EditboxWindow->Child->Name.Buffer[ OriginalLength ] );

	return STATUS_SUCCESS;
}


NTSTATUS
NtGdiWriteConsole(
	__in HANDLE ConsoleHandle,
	__in PWCHAR Buffer,
	__in ULONG32 Length
)
{

	KeProbeForRead( Buffer, Length * sizeof( WCHAR ) );

	NTSTATUS ntStatus;
	PKCONSOLE Console;

	ntStatus = ObReferenceObjectByHandle( ConsoleHandle, &Console );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	POBJECT_TYPE_DESCRIPTOR Type;

	ntStatus = ObQueryObjectType( Console, &Type );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	if ( Type != ObjectTypeConsole ) {

		return STATUS_INVALID_HANDLE;
	}

	PKWINDOW EditboxWindow = Console->Console->Child;

	for ( ULONG32 i = 0; i < Length; i++ ) {

		if ( ( WCHAR )Buffer[ i ] == '\b' ) {

			if ( EditboxWindow->Name.Length > 0 ) {
				EditboxWindow->Name.Length--;
				EditboxWindow->Name.Buffer[ EditboxWindow->Name.Length ] = ( WCHAR )0;
			}
		}
		else {

			KeAcquireSpinLock( &EditboxWindow->CompositeLock );
			EditboxWindow->Name.Buffer[ EditboxWindow->Name.Length ] = ( WCHAR )Buffer[ i ];
			EditboxWindow->Name.Length++;
			EditboxWindow->Name.Buffer[ EditboxWindow->Name.Length ] = 0;

			if ( ( EditboxWindow->Name.Length * sizeof( WCHAR ) ) >= EditboxWindow->Name.Size ) {

				PWCHAR NewBuffer = MmAllocateMemory( EditboxWindow->Name.Size + 0x1000, PAGE_READ | PAGE_WRITE );

				_memcpy( NewBuffer, EditboxWindow->Name.Buffer, EditboxWindow->Name.Size );

				MmFreeMemory( ( ULONG64 )EditboxWindow->Name.Buffer, EditboxWindow->Name.Size );

				EditboxWindow->Name.Buffer = NewBuffer;
				EditboxWindow->Name.Size += 0x1000;
			}
			KeReleaseSpinLock( &EditboxWindow->CompositeLock );
		}

	}

	//_memcpy( EditboxWindow->Name.Buffer + EditboxWindow->Name.Length, Buffer, Length );
#if 0
	for ( ULONG32 i = 0; i < Length; i++ ) {

		EditboxWindow->Class->WndProc( EditboxWindow, WM_KEY_PRESS, ( ULONG32 )Buffer[ i ], 0 );
	}
#endif

	return STATUS_SUCCESS;
}