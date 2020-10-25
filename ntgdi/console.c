

#include "driver.h"
#include "wnd.h"




POBJECT_TYPE_DESCRIPTOR ObjectTypeConsole = 0;

typedef struct _KCONSOLE {

	PWCHAR  InputBuffer;
	ULONG32 InputBufferLength;
	ULONG32 InputBufferIndex;

	PWCHAR  ReadBuffer;
	ULONG32 ReadBufferLength;
	ULONG32 ReadBufferIndex;

	VOLATILE ULONG32 Flags;

} KCONSOLE, *PKCONSOLE;

NTSTATUS
NtGdiConsoleInitializeSubsystem(

)
{

	NTSTATUS ntStatus;

	ntStatus = ObInitializeObjectType( L"ConsoleObject", sizeof( KCONSOLE ), ' NOC', &ObjectTypeConsole );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	return STATUS_SUCCESS;
}

NTSTATUS
NtGdiConsoleWrite(
	__in HANDLE ConsoleHandle,
	__in PWCHAR Buffer,
	__in ULONG32 Length
)
{

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

	Length -= 2;
	_memcpy( ( void* )( ( char* )Console->InputBuffer + Console->InputBufferIndex ), Buffer, Length );
	Console->InputBufferIndex += Length;

	return STATUS_SUCCESS;
}


