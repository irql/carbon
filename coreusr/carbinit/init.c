


#include <carbusr.h>
#include "../../ddi/d3d/d3d.h"

#include <ft2build.h>
#include FT_FREETYPE_H

int _fltused = 0;

FT_Library FreeTypeLibrary;

VOID
NtProcessStartup(

)
{
    HANDLE WindowHandle;
    HANDLE ButtonHandle;
    HANDLE StaticHandle;

    WND_CLASS WindowClass;

    KSYSTEM_TIME SystemTime;
    WCHAR BootString[ 512 ];

    LdrInitializeProcess( );

    char data[ 256 ] = { 0 };

    FILE* file;
    file = fopen( "HI.TXT", "r" );
    fseek( file, 5, SEEK_SET );
    if ( file )
        fread( data, 256, 1, file );

    NtQuerySystemClock( &SystemTime );

    fseek( file, 0, SEEK_END );

    swprintf( BootString,
              L"booted on %.2d/%.2d/20%.2d at %.2d:%.2d:%.2d\n"
              L"THE REAL MALLOC: 0x%.8ull\n"
              L"hi.txt reads: %as\n"
              L"size: %d",
              SystemTime.Day,
              SystemTime.Month,
              SystemTime.Year,
              SystemTime.Hour,
              SystemTime.Minute,
              SystemTime.Second,
              malloc( 500 ),
              data,
              ftell( file ) );

    RtlDebugPrint( BootString );

    wcscpy( WindowClass.ClassName, L"BITCH" );
    WindowClass.WndProc = NULL;

    FT_Error Error;
    static FT_Face  FontFace;

    RtlDebugPrint( L"Bogster.\n" );
    Error = FT_Init_FreeType( &FreeTypeLibrary );
    RtlDebugPrint( L"Init: %d\n", Error );


#if 0
    Error = FT_New_Face( FreeTypeLibrary,
                         "C:/SYSTEM/FONTS/COUSINE.TTF",
                         0,
                         &FontFace );
#else

    FILE* font = fopen( "C:/SYSTEM/FONTS/ARIAL.TTF", "rb" );
    HANDLE sect;
    OBJECT_ATTRIBUTES oa = { 0 };
    PVOID Mapped = NULL;

    fseek( font, 0, SEEK_END );
    ULONG Size = ftell( font );

    NtCreateSection( &sect, SECTION_MAP_READ, &oa, 0, font->FileHandle );
    NtMapViewOfSection( sect, NtCurrentProcess( ), &Mapped, 0, 0, PAGE_READ );

    RtlDebugPrint( L"LastByte: %ul\n", *( ( PCHAR )Mapped + Size - 1 ) );
    RtlDebugPrint( L"Mapped at %ull %ull\n", Mapped, Size );

    Error = FT_New_Memory_Face( FreeTypeLibrary,
                                Mapped,
                                Size,
                                0,
                                &FontFace );
    RtlDebugPrint( L"Face: %d\n", Error );
#endif

    /*
    Error = FT_Set_Char_Size( FontFace,
                              0,
                              16 * 64,
                              114,
                              23 );*/
    Error = FT_Set_Pixel_Sizes( FontFace,
                                0,
                                16 );
    RtlDebugPrint( L"Char: %d\n", Error );

    NtRegisterClass( &WindowClass );

    //
    // TODO: move most if not all of the classes
    // into user.dll
    //

    NtCreateWindow( &WindowHandle,
                    0,
                    L"window title",
                    L"BITCH",
                    50,
                    50,
                    400,
                    300,
                    0 );
    NtCreateWindow( &ButtonHandle,
                    WindowHandle,
                    L"button",
                    L"BUTTON",
                    280,
                    24,
                    114,
                    23,
                    0 );
    NtCreateWindow( &StaticHandle,
                    WindowHandle,
                    BootString,
                    L"STATIC",
                    5,
                    24,
                    270,
                    100,
                    0 );

    KUSER_MESSAGE Message;
    HANDLE ContextHandle;

    static ULONG32 bits[ ] = {
        0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000,
        0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000,
        0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000,
        0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000,
    };


    while ( TRUE ) {

        NtWaitMessage( WindowHandle );

        if ( NtReceiveMessage( WindowHandle, &Message ) ) {
            NtDefaultWindowProc( WindowHandle,
                                 Message.MessageId,
                                 Message.Param1,
                                 Message.Param2 );
        }

        if ( NtReceiveMessage( ButtonHandle, &Message ) ) {

            if ( Message.MessageId == WM_PAINT ) {

                NtBeginPaint( &ContextHandle, ButtonHandle );

                NtDefaultWindowProc( ButtonHandle,
                                     Message.MessageId,
                                     Message.Param1,
                                     Message.Param2 );
#if 1
                FT_GlyphSlot Slot;
                Slot = FontFace->glyph;

                Error = FT_Load_Char( FontFace, 'p', FT_LOAD_RENDER );

                //RtlDebugPrint( L"Load Error: %d\n", Error );

                //RtlDebugPrint( L"Pogin %d, %d\n", Slot->bitmap_left, Slot->bitmap_top );

                NtBltBits( &Slot->bitmap,
                           0,
                           0,
                           Slot->bitmap_left,
                           Slot->bitmap_top,
                           ContextHandle,
                           0,
                           0 );
#endif
                //NtBltBits( bits, 0, 0, 4, 4, ContextHandle, 0, 0 );

                NtEndPaint( ButtonHandle );
            }
            else {
                NtDefaultWindowProc( ButtonHandle,
                                     Message.MessageId,
                                     Message.Param1,
                                     Message.Param2 );
            }
        }


        if ( NtReceiveMessage( StaticHandle, &Message ) ) {
            NtDefaultWindowProc( StaticHandle,
                                 Message.MessageId,
                                 Message.Param1,
                                 Message.Param2 );
        }
    }
}
