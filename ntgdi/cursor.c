

#include "driver.h"

UNICODE_STRING g_DefaultCursorDirectoryFile = RTL_CONSTANT_UNICODE_STRING( L"\\SystemRoot\\cur.bmp" );

KCURSOR g_Cursor;

//
//	maybe add a list of cursors, with their bitmap headers. and switch between these.
//

NTSTATUS
NtInitializeCursor(

)
{

	NTSTATUS ntStatus;
	HANDLE FileHandle;

	OBJECT_ATTRIBUTES ObjectAttributes = { 0, NULL };
	ObjectAttributes.ObjectName = &g_DefaultCursorDirectoryFile;

	IO_STATUS_BLOCK Iosb;

	ntStatus = ZwCreateFile( &FileHandle, &Iosb, GENERIC_READ | GENERIC_WRITE, 0, &ObjectAttributes );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	if ( !NT_SUCCESS( Iosb.Status ) ) {

		return Iosb.Status;
	}

	FILE_BASIC_INFORMATION BasicInfo;
	ntStatus = ZwQueryInformationFile( FileHandle, &Iosb, &BasicInfo, sizeof( FILE_BASIC_INFORMATION ), FileBasicInformation );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	if ( !NT_SUCCESS( Iosb.Status ) ) {

		return Iosb.Status;
	}

	PBITMAP_HEADER CursorBitmapHeader = ( PBITMAP_HEADER )MmAllocateMemory( BasicInfo.FileSize, PAGE_READ | PAGE_WRITE );

	ntStatus = ZwReadFile( FileHandle, &Iosb, ( PVOID )CursorBitmapHeader, ( ULONG32 )BasicInfo.FileSize, 0 );

	if ( !NT_SUCCESS( ntStatus ) ) {

		MmFreeMemory( ( ULONG64 )CursorBitmapHeader, BasicInfo.FileSize );
		return ntStatus;
	}

	if ( !NT_SUCCESS( Iosb.Status ) ) {

		MmFreeMemory( ( ULONG64 )CursorBitmapHeader, BasicInfo.FileSize );
		return Iosb.Status;
	}

	ULONG32* CursorBitmapPixels = ( ULONG32* )( ( PUCHAR )CursorBitmapHeader + CursorBitmapHeader->ImageOffset );

	g_Cursor.Width = CursorBitmapHeader->Width;
	g_Cursor.Height = CursorBitmapHeader->Height;

	POINT CurrentPixel = { 0, CursorBitmapHeader->Height };
	for ( ULONG32 i = 0; i <= CursorBitmapHeader->Height * CursorBitmapHeader->Width; i++ ) {

		g_Cursor.Buffer[ CurrentPixel.y * CursorBitmapHeader->Height + CurrentPixel.x ] = CursorBitmapPixels[ i ];

		CurrentPixel.x++;

		if ( ( CurrentPixel.x ) >= CursorBitmapHeader->Width ) {

			CurrentPixel.y--;
			CurrentPixel.x = 0;
		}
	}

	return STATUS_SUCCESS;
}