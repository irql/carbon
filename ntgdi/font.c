


#include "driver.h"

//
//	everything in this file is a template.
//

POBJECT_TYPE_DESCRIPTOR ObjectTypeFont;

PKFONT g_DefaultFont;

NTSTATUS
NtGdiFontInitializeObject(

)
{
	NTSTATUS ntStatus;

	ntStatus = ObInitializeObjectType( L"FontObject", sizeof( KFONT ), 'TNOF', &ObjectTypeFont );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	HANDLE FontHandle;
	ntStatus = NtGdiCreateFont( &FontHandle, NULL );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	ntStatus = ObReferenceObjectByHandle( FontHandle, &g_DefaultFont );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	ZwClose( FontHandle );

	return STATUS_SUCCESS;
}

NTSTATUS
NtGdiInitializeFont(
	__in PKFONT FontObject
)
{
	//
	//	this is when it is decided what the type is and the render, extentpoint fields are filled.
	//

	if ( FontObject->FileBuffer == NULL ) {

		FontObject->FileBuffer = MmAllocateMemory( 4096, PAGE_READ | PAGE_WRITE );
		FontObject->FileSize = 4096;

		return NtGdiInitializeBitFont( FontObject );
	}

	return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NtGdiCreateFont(
	__out PHANDLE FontHandle,
	__in PUNICODE_STRING FontFile
)
{
	STATIC OBJECT_ATTRIBUTES DefaultAttributes = { OBJ_PERMANENT, NULL };

	NTSTATUS ntStatus;

	HANDLE FontFileHandle = 0;
	IO_STATUS_BLOCK Iosb;
	OBJECT_ATTRIBUTES FontFileAttributes = { 0 };
	FontFileAttributes.ObjectName = FontFile;

	FILE_BASIC_INFORMATION BasicInfo;

	PKFONT FontObject = NULL;
	ntStatus = ObpCreateObject( &FontObject, &DefaultAttributes, ObjectTypeFont );

	if ( !NT_SUCCESS( ntStatus ) ) {

		goto done;
	}

	if ( FontFile == NULL ) {

		FontObject->FileBuffer = NULL;

		ntStatus = NtGdiInitializeFont( FontObject );

		if ( !NT_SUCCESS( ntStatus ) ) {

			ObDestroyObject( FontObject );
			return ntStatus;
		}

		ntStatus = ObCreateHandle( FontHandle, FontObject );

		if ( !NT_SUCCESS( ntStatus ) ) {

			ObDestroyObject( FontObject );
			return ntStatus;
		}

		return STATUS_SUCCESS;
	}

	ntStatus = ZwCreateFile( &FontFileHandle, &Iosb, GENERIC_READ | GENERIC_WRITE, 0, &FontFileAttributes );

	if ( !NT_SUCCESS( ntStatus ) ) {

		goto done;
	}

	if ( !NT_SUCCESS( Iosb.Status ) ) {

		ntStatus = Iosb.Status;
		goto done;
	}

	ntStatus = ZwQueryInformationFile( FontFileHandle, &Iosb, &BasicInfo, sizeof( FILE_BASIC_INFORMATION ), FileBasicInformation );

	if ( !NT_SUCCESS( ntStatus ) ) {

		goto done;
	}

	if ( !NT_SUCCESS( Iosb.Status ) ) {

		ntStatus = Iosb.Status;
		goto done;
	}

	FontObject->FileBuffer = MmAllocateMemory( BasicInfo.FileSize, PAGE_READ | PAGE_WRITE );
	FontObject->FileSize = BasicInfo.FileSize;

	ntStatus = ZwReadFile( FontFileHandle, &Iosb, FontObject->FileBuffer, FontObject->FileSize, 0 );

	ZwClose( FontFileHandle );

	if ( !NT_SUCCESS( ntStatus ) ) {

		goto done;
	}

	if ( !NT_SUCCESS( Iosb.Status ) ) {

		ntStatus = Iosb.Status;
		goto done;
	}

	NtGdiInitializeFont( FontObject );

	ntStatus = ObCreateHandle( FontHandle, FontObject );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

done:;

	if ( !NT_SUCCESS( ntStatus ) ) {

		if ( FontObject ) {

			if ( FontObject->FileBuffer ) {

				MmFreeMemory( ( ULONG64 )FontObject->FileBuffer, FontObject->FileSize );
			}

			ObDestroyObject( FontObject );
		}
	}

	if ( FontFileHandle ) {

		ZwClose( FontFileHandle );
	}

	return ntStatus;
}

NTSTATUS
NtGdiFontExtentPoint(
	__in HANDLE FontHandle,
	__in PWCHAR Buffer,
	__in PPOINT Region,
	__in PPOINT ExtentPoint
)
{

	NTSTATUS ntStatus;
	PKFONT FontObject;

	ntStatus = ObReferenceObjectByHandle( FontHandle, &FontObject );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	POBJECT_TYPE_DESCRIPTOR Type;

	ntStatus = ObQueryObjectType( FontObject, &Type );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	if ( Type != ObjectTypeFont ) {

		return STATUS_INVALID_HANDLE;
	}

	FontObject->ExtentPoint( FontObject, Buffer, Region, ExtentPoint );

	return STATUS_SUCCESS;
}

NTSTATUS
NtGdiFontEndPoint(
	__in HANDLE FontHandle,
	__in PWCHAR Buffer,
	__in PPOINT Region,
	__in PPOINT EndPoint
)
{

	NTSTATUS ntStatus;
	PKFONT FontObject;

	ntStatus = ObReferenceObjectByHandle( FontHandle, &FontObject );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	POBJECT_TYPE_DESCRIPTOR Type;

	ntStatus = ObQueryObjectType( FontObject, &Type );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	if ( Type != ObjectTypeFont ) {

		return STATUS_INVALID_HANDLE;
	}

	FontObject->EndPoint( FontObject, Buffer, Region, EndPoint );

	return STATUS_SUCCESS;
}
