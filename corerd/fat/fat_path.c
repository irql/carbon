


#include "fat.h"

//
// Pasted from original carbon.
//

FAT_PATH_TYPE
FspValidateFileName(
    _In_ PWCHAR FileName
)
{
    FAT_PATH_TYPE Type;
    ULONG64 Length;
    ULONG64 Dot;
    ULONG64 Char;

    Type = Path8Dot3;
    Length = RtlStringLength( FileName );
    Dot = Length + 1;

    for ( Char = 0; Char < Length; Char++ ) {

        //
        // 0x05 is to be replaced with 0xE5
        //

        if ( FileName[ Char ] != 0x05 &&
             FileName[ Char ] < 0x20 ) {

            return PathInvalid;
        }

        if ( Type == Path8Dot3 && FileName[ Char ] > 0x7E ) {

            Type = PathLongFileName;
        }

        //
        // UCS-2, not UTF16LE, we need to filter out 
        // surrogate pairs, they have a CP between
        // D8 and DF.
        //

        if ( FileName[ Char ] >= 0xD800 &&
             FileName[ Char ] <= 0xDF00 ) {

            return PathInvalid;
        }

        if ( FileName[ Char ] == '.' ) {

            Dot = Char;
        }

    }

    if ( Type == Path8Dot3 ) {

        if ( Dot != Length + 1 ) {

            if ( Dot > 9 ) {

                Type = PathLongFileName;
            }
        }

        if ( Length > 12 ) {

            Type = PathLongFileName;
        }

    }

    return Type;
}

VOID
FspConvertPathTo8Dot3(
    _In_  PWCHAR FileName,
    _Out_ PCHAR  FileName8Dot3
)
{
    ULONG64 Length;
    ULONG64 Dot;
    ULONG64 Char;

    Length = RtlStringLength( FileName );
    Dot = Length + 1;

    for ( Char = Length; Char > 0; Char-- ) {

        if ( FileName[ Char ] == '.' ) {

            Dot = Char;
            break;
        }
    }

    memset( FileName8Dot3, ' ', 11 );
    FileName8Dot3[ 11 ] = 0;

    for ( Char = 0; Char < Dot && FileName[ Char ] != 0; Char++ ) {

        FileName8Dot3[ Char ] = ( CHAR )RtlUpperChar( FileName[ Char ] );
    }

    if ( Dot < Length ) {

        Dot++;
        //for ( Char = Dot; Char - Dot < 3; Char++ ) {
        //for ( Char = Dot; Char - Dot < 3; Char++ ) {

            //FileName8Dot3[ ( Dot <= 8 ? 8 : Dot ) + Char - Dot ] = ( CHAR )RtlUpperChar( FileName[ Char ] );
        //}
        for ( Char = Dot; Char - Dot < 3; Char++ ) {
            FileName8Dot3[ Char - Dot + 8 ] = ( CHAR )RtlUpperChar( FileName[ Char ] );
        }
    }
}

UCHAR
FspNameChecksum(
    _In_ PCHAR ShortName
)
{
    UCHAR Checksum = 0;

    for ( UCHAR Length = 11; Length != 0; Length-- ) {

        Checksum = ( ( Checksum & 1 ) ? 0x80 : 0 ) + ( Checksum >> 1 ) + *ShortName++;
    }

    return Checksum;
}

LONG
FspCompareLfnEntry(
    _In_ PFAT_DIRECTORY Directory,
    _In_ PWCHAR         Chars
)
{
    LONG Return;
    ULONG64 Length;
    ULONG64 Char;

    Length = RtlStringLength( Chars );

    if ( Length > 13 ) {

        Length = 13;
    }

    if ( Length <= 5 ) {

        return RtlCompareStringLength( Directory->Long.First5Chars, Chars, TRUE, Length );
    }

    Return = RtlCompareStringLength( Directory->Long.First5Chars, Chars, TRUE, 5 );
    if ( Return != 0 ) {

        return Return;
    }

    Length -= 5;

    if ( Length <= 6 ) {

        return RtlCompareStringLength( Directory->Long.Next6Chars, Chars + 5, TRUE, Length );
    }

    Return = RtlCompareStringLength( Directory->Long.Next6Chars, Chars + 5, TRUE, 6 );
    if ( Return != 0 ) {

        return Return;
    }

    Length -= 6;

    for ( Char = 0; Char < Length; Char++ ) {

        Return = Directory->Long.Next2Chars[ Char ] - Chars[ 11 + Char ];

        if ( Return != 0 ) {

            return Return;
        }
    }

    return 0;
}

ULONG64
FspFindDirectoryFile(
    _In_ PFAT_DIRECTORY Directory,
    _In_ FAT_PATH_TYPE  Type,
    _In_ PVOID          FileName
)
{
    if ( Type == Path8Dot3 ) {
        PCHAR FileName8Dot3;
        ULONG64 Char;

        FileName8Dot3 = FileName;

        for ( Char = 0; Directory[ Char ].Short.Name[ 0 ] != 0; Char++ ) {

            if ( ( UCHAR )Directory[ Char ].Short.Name[ 0 ] == ( UCHAR )FAT32_DIRECTORY_ENTRY_FREE ) {

                continue;
            }

            if ( Directory[ Char ].Short.Attributes & FAT32_VOLUME_ID ) {

                continue;
            }

            if ( RtlCompareAnsiStringLength( Directory[ Char ].Short.Name, FileName8Dot3, TRUE, 11 ) == 0 ) {

                return Char;
            }
        }

        return ( ULONG64 )-1;
    }
    else if ( Type == PathLongFileName ) {
        PWCHAR FileNameLfn;
        ULONG64 LongNameFound;
        ULONG64 ShortNameFound;
        ULONG64 Entry;
        ULONG64 EntryShort;
        ULONG64 FileNameIndex;

        FileNameLfn = FileName;
        LongNameFound = ( ULONG64 )-1;
        ShortNameFound = ( ULONG64 )-1;

        for ( Entry = 0; Directory[ Entry ].Long.OrderOfEntry != 0; Entry++ ) {

            if ( Directory[ Entry ].Long.Attributes != FAT32_LFN )
                continue;

            if ( ( Directory[ Entry ].Long.OrderOfEntry & ~( UCHAR )FAT32_LAST_LFN_ENTRY ) != 0x1 )
                continue;

            EntryShort = Entry;
            FileNameIndex = 0;
            do {

                if ( FspCompareLfnEntry( Directory + Entry, ( PWCHAR )( FileNameLfn + FileNameIndex ) ) != 0 ) {

                    break;
                }

                if ( Directory[ Entry ].Long.OrderOfEntry & FAT32_LAST_LFN_ENTRY ) {
                    LongNameFound = Entry;
                    ShortNameFound = EntryShort + 1;
                    break;
                }
                Entry--;

                FileNameIndex += 13;
            } while ( Entry != ( ULONG64 )-1 );

            if ( LongNameFound != ( ULONG64 )-1 ) {

                break;
            }

            Entry = EntryShort;
        }

        if ( LongNameFound == ( ULONG64 )-1 ) {

            return LongNameFound;
        }

        if ( FspNameChecksum( ( PCHAR )&Directory[ ShortNameFound ].Short.Name ) == Directory[ LongNameFound ].Long.NameChecksum ) {

            return ShortNameFound;
        }

        //Fucking Orphan.
        return ( ULONG64 )-1;
    }
    else {

        return ( ULONG64 )-1;
    }
}
