


#define USER_INTERNAL
#include <carbusr.h>

#include <ft2build.h>
#include FT_FREETYPE_H

typedef struct _NT_FONT_HANDLE {

    //
    // Fonts are all user mode and are driven by 
    // this dll, which wraps FreeType
    //

    //
    // This should probably be moved into some peb
    // structure, or maybe even the teb? freetype mentions
    // this being per-thread
    //

    FT_Library FreeType;
    FT_Face    FontFace;

    //
    // should FontFile really exist?
    //

    HANDLE     FontFile;
    HANDLE     FontSection;

    PVOID      FontMap;
    ULONG64    FontLength;

} NT_FONT_HANDLE, *PNT_FONT_HANDLE;

NTSTATUS
NtCreateFont(
    _Out_ PNT_FONT_HANDLE* FontHandle,
    _In_  ULONG32          Height,
    _In_  ULONG32          Width,
    _In_  PWSTR            FaceName
)
{
    NTSTATUS ntStatus;
    HANDLE SectionHandle;
    HANDLE FileHandle;
    OBJECT_ATTRIBUTES File = { RTL_CONSTANT_STRING( L"\\??\\BootDevice" ), { 0 }, 0 };
    OBJECT_ATTRIBUTES Section = { 0 };
    IO_STATUS_BLOCK StatusBlock = { 0 };
    PVOID FontMap;
    ULONG64 FontLength;
    FILE_BASIC_INFORMATION Basic;
    PNT_FONT_HANDLE Font;

    FT_Library Library;
    FT_Face Face;
    FT_Error Error;

    RTL_STACK_STRING( File.RootDirectory, 256 );

    FileHandle = 0;
    SectionHandle = 0;
    FontMap = NULL;
    FontLength = 0;

    wcscpy( File.RootDirectory.Buffer, L"\\SYSTEM\\FONTS\\" );
    wcscat( File.RootDirectory.Buffer, FaceName );

    File.RootDirectory.Length = ( USHORT )wcslen( File.RootDirectory.Buffer ) * sizeof( WCHAR );

    ntStatus = NtCreateFile( &FileHandle,
                             &StatusBlock,
                             GENERIC_ALL | SYNCHRONIZE,//GENERIC_READ | SYNCHRONIZE,
                             &File,
                             FILE_OPEN_IF,
                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,//FILE_SHARE_READ,
                             0 );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    if ( !NT_SUCCESS( StatusBlock.Status ) ) {

        return StatusBlock.Status;
    }

    ntStatus = NtQueryInformationFile( FileHandle,
                                       &StatusBlock,
                                       &Basic,
                                       sizeof( FILE_BASIC_INFORMATION ),
                                       FileBasicInformation );
    if ( !NT_SUCCESS( ntStatus ) ) {

        NtClose( FileHandle );
        return ntStatus;
    }

    if ( !NT_SUCCESS( StatusBlock.Status ) ) {

        NtClose( FileHandle );
        return StatusBlock.Status;
    }
    FontLength = Basic.FileLength;

    ntStatus = NtCreateSection( &SectionHandle,
                                SECTION_ALL_ACCESS,//SECTION_MAP_READ,
                                &Section,
                                SEC_READ,
                                FileHandle );
    if ( !NT_SUCCESS( ntStatus ) ) {

        NtClose( FileHandle );
        return ntStatus;
    }

    ntStatus = NtMapViewOfSection( SectionHandle,
                                   NtCurrentProcess( ),
                                   &FontMap,
                                   0,
                                   0,
                                   PAGE_READ );
    if ( !NT_SUCCESS( ntStatus ) ) {

        NtClose( SectionHandle );
        NtClose( FileHandle );
        return ntStatus;
    }

    RtlDebugPrint( L"mapped at %ull\n", FontMap );

    Error = FT_Init_FreeType( &Library );

    if ( Error != 0 ) {

        NtUnmapViewOfSection( NtCurrentProcess( ), FontMap );
        NtClose( SectionHandle );
        NtClose( FileHandle );
        return STATUS_INVALID_IMAGE;
    }

    Error = FT_New_Memory_Face( Library,
                                FontMap,
                                ( FT_Long )FontLength,
                                0,
                                &Face );
    if ( Error != 0 ) {

        FT_Done_FreeType( Library );
        NtUnmapViewOfSection( NtCurrentProcess( ), FontMap );
        NtClose( SectionHandle );
        NtClose( FileHandle );
        return STATUS_INVALID_IMAGE;
    }

    Error = FT_Set_Pixel_Sizes( Face,
                                Width,
                                Height );
    if ( Error != 0 ) {

        FT_Done_Face( Face );
        FT_Done_FreeType( Library );
        NtUnmapViewOfSection( NtCurrentProcess( ), FontMap );
        NtClose( SectionHandle );
        NtClose( FileHandle );
        return STATUS_INVALID_IMAGE; // look at all these boilers...
    }

    Font = RtlAllocateHeap( NtCurrentPeb( )->ProcessHeap,
                            sizeof( NT_FONT_HANDLE ) );

    if ( Font == NULL ) {

        FT_Done_Face( Face );
        FT_Done_FreeType( Library );
        NtUnmapViewOfSection( NtCurrentProcess( ), FontMap );
        NtClose( SectionHandle );
        NtClose( FileHandle );
        return ntStatus;
    }

    Font->FreeType = Library;
    Font->FontFace = Face;
    Font->FontMap = FontMap;
    Font->FontLength = FontLength;
    Font->FontFile = FileHandle;
    Font->FontSection = SectionHandle;

    *FontHandle = Font;

    return STATUS_SUCCESS;
}

NTSTATUS
NtDrawText(
    _In_ HANDLE          ContextHandle,
    _In_ PNT_FONT_HANDLE FontHandle,
    _In_ PWSTR           DrawText,
    _In_ PRECT           Rect,
    _In_ ULONG32         Flags,
    _In_ ULONG32         Colour
)
{
    Flags;

    FT_Error Error;
    LONG32 Left = Rect->Left;
    LONG32 Top = Rect->Top;
    ULONG32 Bits[ 2048 ];

    while ( *DrawText ) {

        Error = FT_Load_Char( FontHandle->FontFace,
                              *DrawText,
                              FT_LOAD_RENDER | FT_LOAD_MONOCHROME );

        if ( Error != 0 ) {

            return STATUS_UNSUCCESSFUL;
        }

        FT_GlyphSlot sl = FontHandle->FontFace->glyph;
        FT_Bitmap bm = sl->bitmap;

        for ( unsigned int y = 0; y < bm.rows; y++ ) {

            for ( int byte_index = 0; byte_index < bm.pitch; byte_index++ ) {

                unsigned char byte_value = bm.buffer[ y * bm.pitch + byte_index ];

                unsigned int num_bits_done = byte_index * 8;

                unsigned int row_start = y * bm.width + byte_index * 8;

                for ( unsigned int bit_index = 0;
                      bit_index < min( 8, bm.width - num_bits_done );
                      bit_index++ ) {

                    unsigned char bit = byte_value & ( 1 << ( 7 - bit_index ) );

                    Bits[ row_start + bit_index ] = bit ? Colour : 0;
                }
            }
        }

        NtBltBits( Bits,
                   0,
                   0,
                   bm.width,
                   bm.rows,
                   ContextHandle,
                   Left + sl->bitmap_left,
                   Top - sl->bitmap_top );

        Left += sl->advance.x / 64;
        Top += sl->advance.y / 64;
        DrawText++;
    }

    return STATUS_SUCCESS;
}
