


#include <carbsup.h>
#include "usersup.h"
#include "ntuser.h"

//
// before I even write this, move to user mode
//
// wait why did i write this in kernel mode when its gona be in user mode
//

POBJECT_TYPE NtFontObject;

#define ENGINE_RASTER    0
#define ENGINE_TRUE_TYPE 1
#define ENGINE_OPEN_TYPE 2
#define ENGINE_VECTOR    3

KFONT_ENGINE Engine[ ] = {
    { NtRenderRasterFont, NtExtentRasterFont },
};

VOID
NtInitializeUserFonts(

)
{
    UNICODE_STRING FontObjectName = RTL_CONSTANT_STRING( L"Font" );

    ObCreateObjectType( &NtFontObject,
                        &FontObjectName,
                        USER_TAG,
                        NULL );

}

NTSTATUS
NtCreateFont(
    _Out_ PHANDLE FontHandle,
    _In_  PWSTR   FontFace,
    _In_  ULONG   FontWidth,
    _In_  ULONG   FontHeight,
    _In_  ULONG   FontFlags
)
{
    NTSTATUS ntStatus;
    PKFONT FontObject;
    OBJECT_ATTRIBUTES Font = { 0 };
    OBJECT_ATTRIBUTES File = { RTL_CONSTANT_STRING( L"\\??\\BootDevice" ), { 0 }, OBJ_KERNEL_HANDLE };
    OBJECT_ATTRIBUTES Section = { { 0 }, { 0 }, OBJ_KERNEL_HANDLE };
    IO_STATUS_BLOCK StatusBlock;
    HANDLE ProcessHandle;

    RTL_STACK_STRING( File.RootDirectory, 256 );

    ntStatus = ObCreateObject( &FontObject,
                               NtFontObject,
                               &Font,
                               sizeof( KFONT ) );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    lstrcpyW( File.RootDirectory.Buffer, L"\\SYSTEM\\FONTS\\" );
    lstrcatW( File.RootDirectory.Buffer, FontFace );
    File.RootDirectory.Length = ( USHORT )lstrlenW( File.RootDirectory.Buffer ) * sizeof( WCHAR );

    ntStatus = ZwCreateFile( &FontObject->FileHandle,
                             &StatusBlock,
                             GENERIC_ALL | SYNCHRONIZE,
                             &File,
                             FILE_OPEN_IF,
                             FILE_SHARE_READ,
                             0 );
    if ( !NT_SUCCESS( ntStatus ) ||
         !NT_SUCCESS( StatusBlock.Status ) ) {

        ObDereferenceObject( FontObject );
        return ntStatus | StatusBlock.Status;
    }

    FontObject->FontFlags = FONT_RASTER;
    FontObject->Engine = &Engine[ ENGINE_RASTER ];
    FontObject->FontFlags = FontFlags;
    FontObject->FontWidth = FontWidth;
    FontObject->FontHeight = FontHeight;
    FontObject->FileMapping = NULL;

    ntStatus = ZwCreateSection( &FontObject->SectionHandle,
                                SECTION_ALL_ACCESS,
                                &Section,
                                SEC_IMAGE_NO_EXECUTE,
                                FontObject->FileHandle );
    if ( !NT_SUCCESS( ntStatus ) ) {

        ZwClose( FontObject->FileHandle );
        ObDereferenceObject( FontObject );
        return ntStatus;
    }

    // these stairs are in memory of xerox
    NT_ASSERT(
        NT_SUCCESS(
            ObOpenObjectFromPointer( &ProcessHandle,
                                     PsInitialSystemProcess,
                                     PROCESS_ALL_ACCESS,
                                     OBJ_KERNEL_HANDLE,
                                     KernelMode ) ) );

    ntStatus = ZwMapViewOfSection( FontObject->SectionHandle,
                                   ProcessHandle,
                                   &FontObject->FileMapping,
                                   0,
                                   0,
                                   PAGE_READ );
    if ( !NT_SUCCESS( ntStatus ) ) {

        ZwClose( ProcessHandle );
        ZwClose( FontObject->SectionHandle );
        ZwClose( FontObject->FileHandle );
        ObDereferenceObject( FontObject );
        return ntStatus;
    }

    ntStatus = ObOpenObjectFromPointer( FontHandle,
                                        FontObject,
                                        0,
                                        0,
                                        UserMode );
    if ( !NT_SUCCESS( ntStatus ) ) {

        ZwUnmapViewOfSection( ProcessHandle,
                              FontObject->FileMapping );
        ZwClose( ProcessHandle );
        ZwClose( FontObject->SectionHandle );
        ZwClose( FontObject->FileHandle );
        ObDereferenceObject( FontObject );
        return ntStatus;
    }

    ObDereferenceObject( FontObject );
    return STATUS_SUCCESS;
}

NTSTATUS
NtRenderFont(
    _In_ HANDLE FontHandle,
    _In_ HANDLE ContextHandle,
    _In_ PWSTR  String,
    _In_ PRECT  Clip,
    _In_ ULONG  Color
)
{
    NTSTATUS ntStatus;
    PDC ContextObject;
    PKFONT FontObject;

    ntStatus = ObReferenceObjectByHandle( &FontObject,
                                          FontHandle,
                                          0,
                                          UserMode,
                                          NtFontObject );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    ntStatus = ObReferenceObjectByHandle( &ContextObject,
                                          ContextHandle,
                                          0,
                                          UserMode,
                                          NtDeviceContext );
    if ( !NT_SUCCESS( ntStatus ) ) {

        ObDereferenceObject( FontObject );
        return ntStatus;
    }

    FontObject->Engine->Render( FontObject,
                                ContextObject,
                                String,
                                Clip,
                                Color );

    ObDereferenceObject( FontObject );
    ObDereferenceObject( ContextObject );
    return STATUS_SUCCESS;
}

VOID
NtRenderRasterFont(
    _In_ PKFONT FontObject,
    _In_ PDC    ContextObject,
    _In_ PWSTR  String,
    _In_ PRECT  Clip,
    _In_ ULONG  Color
)
{
    ULONG32 x;
    ULONG32 y;

    ULONG64 i;
    ULONG64 j;
    ULONG64 k;
    UCHAR   Line;

    x = Clip->Left;
    y = Clip->Top;

    for ( k = 0; String[ k ] != 0; k++ ) {

        switch ( String[ k ] ) {
        case '\n':

            x = Clip->Left;
            y += FontObject->FontHeight;
            break;
        default:

            for ( i = 0; i < FontObject->FontHeight; i++ ) {
                Line = ( ( UCHAR* )FontObject->FileMapping )[ String[ k ] * FontObject->FontHeight + i ];
                for ( j = 0; j <= FontObject->FontWidth; j++ ) {
                    if ( ( Line >> ( FontObject->FontWidth - j - 1 ) ) & 1 ) {

                        if (
                            x + j < ( ULONG32 )Clip->Right &&
                            y + i < ( ULONG32 )Clip->Bottom ) {
                            NtDdiSetPixel( ContextObject,
                                           x + ( ULONG32 )j,
                                           y + ( ULONG32 )i,
                                           Color );
                        }

                    }
                }
            }
            x += FontObject->FontWidth;
            break;
        }
    }
}

VOID
NtExtentRasterFont(
    _In_  PKFONT FontObject,
    _In_  PDC    ContextObject,
    _In_  PWSTR  String,
    _Out_ PRECT  Extent
)
{
    ContextObject;
    FontObject;

    ULONG64 k = 0;

    Extent->Left = 0;
    Extent->Right = 0;
    Extent->Top = 0;
    Extent->Bottom = FontObject->FontHeight;

    for ( k = 0; String[ k ] != 0; k++ ) {

        switch ( String[ k ] ) {
        case '\n':
            Extent->Bottom += FontObject->FontHeight;
            break;
        default:
            Extent->Right += FontObject->FontWidth;
            break;
        }
    }
}
