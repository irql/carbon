


#include <carbsup.h>

VOID
RtlStringReverse(
    _In_ PWSTR String
)
{
    ULONG i, j;
    WCHAR Temp;

    i = 0;
    j = ( ULONG )RtlStringLength( String ) - 1;

    for ( ; i < j; i++, j-- ) {

        Temp = String[ i ];
        String[ i ] = String[ j ];
        String[ j ] = Temp;
    }
}

VOID
RtlIntToString(
    _In_ LONG  Value,
    _In_ PWSTR Buffer,
    _In_ ULONG Base
)
{
    ULONG Temp;
    ULONG Index;
    BOOLEAN Negative;

    Index = 0;
    Negative = FALSE;
    if ( Value == 0 ) {
        Buffer[ Index++ ] = '0';
        Buffer[ Index ] = 0;
        return;
    }

    if ( Base == 10 && Value < 0 ) {
        Negative = TRUE;
        Value = -Value;
    }

    while ( Value != 0 ) {
        Temp = Value % Base;
        Buffer[ Index++ ] = ( Temp > 9 ) ? ( WCHAR )( Temp - 10 ) + 'a' : ( WCHAR )( Temp + '0' );
        Value /= Base;
    }

    if ( Negative ) {
        Buffer[ Index++ ] = '-';
    }

    Buffer[ Index ] = 0;

    RtlStringReverse( Buffer );
}

LONG
RtlStringToInt(
    _In_ PWCHAR Buffer
)
{
    LONG Temp;
    ULONG Index;
    ULONG Base;
    BOOLEAN Negative;

    Base = 10;
    Index = 0;
    Temp = 0;
    Negative = FALSE;
    if ( Buffer[ Index ] == '-' ) {
        Negative = TRUE;
        Index++;
    }

    if ( Buffer[ Index ] == '0' && Buffer[ Index + 1 ] == 'x' ) {
        Base = 16;
        Index += 2;
    }

    for ( ;
          Buffer[ Index ] != 0 &&
          ( ( Buffer[ Index ] >= '0' && Buffer[ Index ] <= '9' ) ||
          ( ( Base > 10 && RtlLowerChar( Buffer[ Index ] ) >= 'a' && RtlLowerChar( Buffer[ Index ] <= 'f' ) ) ) ); Index++ ) {//'a' + base - 10
        if ( Buffer[ Index ] >= '0' && Buffer[ Index ] <= '9' ) {
            Temp = Temp * Base + Buffer[ Index ] - '0';
        }
        else {
            Temp = Temp * Base + Buffer[ Index ] - 'a' + 10;
        }
    }

    if ( Negative ) {
        Temp *= -1;
    }

    return Temp;
}


VOID
RtlFormatBufferFromArgumentList(
    _In_ PWSTR Buffer,
    _In_ PWSTR Format,
    _In_ VA_LIST List
)
{
    ULONG Index;
    BOOLEAN CodeHash;
    BOOLEAN CodePad;
    BOOLEAN CodeUpper;

    ULONG64 ArgULL64;
    ULONG32 Lo32;
    ULONG32 Hi32;
    WCHAR LoBuf[ 16 ];
    WCHAR HiBuf[ 16 ];
    LONG32 LoPad;
    LONG32 HiPad;
    LONG32 ZeroPad;
    PWCHAR ArgWS;
    WCHAR ArgWC;
    PCHAR ArgAS;
    CHAR ArgAC;

    Index = 0;

    while ( *Format ) {
        if ( *Format != '%' ) {
            Buffer[ Index ] = *Format++;
            Index++;
            continue;
        }
        else {
            Format++;
        }

        CodeHash = FALSE;
        CodePad = FALSE;
        CodeUpper = FALSE;
        LoPad = 0;
        HiPad = 0;
        ZeroPad = 0;

        if ( *Format == '#' ) {
            CodeHash = TRUE;
            Format++;
        }

        if ( *Format == '.' ) {
            Format++;
            ZeroPad = RtlStringToInt( Format );
            while ( *Format >= '0' && *Format <= '9' ) {
                Format++;
            }
        }

        switch ( *Format ) {
        case '%':;
            Buffer[ Index ] = *Format++;
            Index++;
            break;
        case 'w':;
            Format++;
        case 's':;
        case 'c':;

            if ( *Format == 's' ) {
                ArgWS = __crt_va_arg( List, PWCHAR );

                if ( ArgWS == NULL ) {
                    ArgWS = L"(null)";
                }
                else if ( *ArgWS == 0 ) {
                    ArgWS = L"(zero)";
                }

                for ( ULONG32 i = 0; ArgWS[ i ]; i++, Index++ ) {
                    Buffer[ Index ] = ArgWS[ i ];
                }

                Format++;
            }
            else if ( *Format == 'c' ) {
                ArgWC = __crt_va_arg( List, WCHAR );

                if ( ArgWC == 0 ) {
                    ArgWS = L"(zero)";
                    for ( ULONG32 i = 0; ArgWS[ i ]; i++, Index++ ) {
                        Buffer[ Index ] = ArgWS[ i ];
                    }
                }
                else {
                    Buffer[ Index++ ] = ArgWC;
                }
                Format++;
            }

            break;
        case 'a':;
            Format++;

            if ( *Format == 's' ) {
                ArgAS = __crt_va_arg( List, PCHAR );

                if ( ArgAS == NULL ) {
                    ArgAS = "(null)";
                }
                else if ( *ArgAS == 0 ) {
                    ArgAS = "(zero)";
                }

                for ( ULONG32 i = 0; ArgAS[ i ]; i++, Index++ ) {
                    Buffer[ Index ] = ( WCHAR )ArgAS[ i ];
                }

                Format++;
            }
            else if ( *Format == 'c' ) {
                ArgAC = __crt_va_arg( List, CHAR );

                if ( ArgAC == 0 ) {
                    ArgAS = "(zero)";
                    for ( ULONG32 i = 0; ArgAS[ i ]; i++, Index++ ) {
                        Buffer[ Index ] = ArgAS[ i ];
                    }
                }
                else {
                    Buffer[ Index++ ] = ArgAC;
                }
                Format++;
            }

            break;
        case 'u':;
        case 'U':;
            Format++;

            if ( ( *Format == 'l' &&
                   *( Format + 1 ) == 'l' ) ||
                   ( *Format == 'L' &&
                     *( Format + 1 ) == 'L' ) ) {
                Format++;

                ArgULL64 = __crt_va_arg( List, ULONG64 );
                Lo32 = ( ULONG32 )( ArgULL64 );
                Hi32 = ( ULONG32 )( ArgULL64 >> 32 );

                CodeUpper = *Format == 'L';

                if ( CodeHash ) {
                    Buffer[ Index++ ] = '0';
                    Buffer[ Index++ ] = 'x';
                }

                RtlIntToString( Lo32, LoBuf, 16 );
                RtlIntToString( Hi32, HiBuf, 16 );
                LoPad = 8 - ( ULONG )RtlStringLength( LoBuf );
                if ( Hi32 == 0 ) {
                    LoPad = 0;
                }

                HiPad = ZeroPad - ( ( ULONG32 )RtlStringLength( LoBuf ) + LoPad );
                if ( Hi32 != 0 ) {
                    HiPad -= ( ULONG )RtlStringLength( HiBuf );
                }

                for ( unsigned char i = 0; i < HiPad; i++, Index++ ) {
                    Buffer[ Index ] = '0';
                }

                for ( unsigned char i = 0; HiBuf[ i ] && Hi32 != 0; i++, Index++ ) {
                    Buffer[ Index ] = CodeUpper ? RtlUpperChar( HiBuf[ i ] ) : HiBuf[ i ];
                }

                for ( unsigned char i = 0; i < LoPad; i++, Index++ ) {
                    Buffer[ Index ] = '0';
                }

                for ( unsigned char i = 0; LoBuf[ i ]; i++, Index++ ) {
                    Buffer[ Index ] = CodeUpper ? RtlUpperChar( LoBuf[ i ] ) : LoBuf[ i ];
                }

                Format++;
            }
            else if ( *Format == 'l' ||
                      *Format == 'L' ) {
                Lo32 = __crt_va_arg( List, ULONG32 );

                CodeUpper = *Format == 'L';

                if ( CodeHash ) {
                    Buffer[ Index++ ] = '0';
                    Buffer[ Index++ ] = 'x';
                }

                RtlIntToString( Lo32, LoBuf, 16 );
                LoPad = ZeroPad - ( LONG32 )RtlStringLength( LoBuf );

                for ( int i = 0; LoPad > 0 && i < LoPad; i++, Index++ ) {
                    Buffer[ Index ] = '0';
                }

                for ( int i = 0; LoBuf[ i ]; i++, Index++ ) {
                    Buffer[ Index ] = CodeUpper ? RtlUpperChar( LoBuf[ i ] ) : LoBuf[ i ];
                }

                Format++;
            }

            break;
        case 'd':
            Lo32 = __crt_va_arg( List, ULONG32 );

            CodeUpper = *Format == 'd';

            if ( CodeHash ) {
                Buffer[ Index++ ] = '0';
                Buffer[ Index++ ] = 'n';
            }

            RtlIntToString( Lo32, LoBuf, 10 );
            LoPad = ZeroPad - ( LONG32 )RtlStringLength( LoBuf );

            for ( unsigned char i = 0; i < LoPad; i++, Index++ ) {
                Buffer[ Index ] = '0';
            }

            for ( unsigned char i = 0; LoBuf[ i ]; i++, Index++ ) {
                Buffer[ Index ] = CodeUpper ? RtlUpperChar( LoBuf[ i ] ) : LoBuf[ i ];
            }

            Format++;
            break;
        default:;
            break;
        }

    }

    Buffer[ Index ] = 0;
}

VOID
RtlFormatBuffer(
    _In_ PWSTR Buffer,
    _In_ PWSTR Format,
    _In_ ...
)
{
    VA_LIST ArgumentList;
    __crt_va_start( ArgumentList, Format );
    RtlFormatBufferFromArgumentList( Buffer, Format, ArgumentList );
    __crt_va_end( ArgumentList );
}

#if 0
VOID
RtlDebugPrint(
    _In_ PWSTR Format,
    _In_ ...
)
{
    STATIC ULONG x = 0, y = 0;
    STATIC PUSHORT Frame = 0;

    if ( Frame == 0 ) {
        Frame = ( PUSHORT )0xb8000;//MmMapIoSpace( 0xb8000, 0x1000 );
    }

    WCHAR Buffer[ 256 ];
    VA_LIST ArgumentList;
    __crt_va_start( ArgumentList, Format );
    RtlFormatBufferFromArgumentList( Buffer, Format, ArgumentList );
    __crt_va_end( ArgumentList );

    ULONG Char = 0;
    while ( Buffer[ Char ] ) {
        if ( Buffer[ Char ] == '\n' ) {
            Char++;
            y++;
            x = 0;
            continue;
        }
        Frame[ y * 80 + x ] = ( ( CHAR )Buffer[ Char++ ] ) | 0x0F00;
        x++;
    }
}
#endif

LONG
RtlCompareString(
    _In_ PCWSTR  String1,
    _In_ PCWSTR  String2,
    _In_ BOOLEAN CaseInsensitive
)
{
    if ( !CaseInsensitive ) {
        while ( *String1 && *String2 && *String1 == *String2 ) {

            String1++, String2++;
        }
        return *String1 - *String2;
    }
    else {
        while ( *String1 && *String2 && RtlUpperChar( *String1 ) == RtlUpperChar( *String2 ) ) {

            String1++, String2++;
        }
        return RtlUpperChar( *String1 ) - RtlUpperChar( *String2 );
    }
}

LONG
RtlCompareStringLength(
    _In_ PCWSTR  String1,
    _In_ PCWSTR  String2,
    _In_ BOOLEAN CaseInsensitive,
    _In_ ULONG64 Length
)
{
    if ( !CaseInsensitive ) {
        while ( Length-- && *String1 && *String2 && *String1 == *String2 ) {

            String1++, String2++;
        }
        return *--String1 - *--String2;
    }
    else {
        while ( Length-- && *String1 && *String2 && RtlUpperChar( *String1 ) == RtlUpperChar( *String2 ) ) {

            String1++, String2++;
        }
        return RtlUpperChar( *--String1 ) - RtlUpperChar( *--String2 );
    }
}

LONG
RtlCompareAnsiString(
    _In_ PCSTR   String1,
    _In_ PCSTR   String2,
    _In_ BOOLEAN CaseInsensitive
)
{
    if ( !CaseInsensitive ) {
        while ( *String1 && *String2 && *String1 == *String2 ) {

            String1++, String2++;
        }
        return *String1 - *String2;
    }
    else {
        while ( *String1 && *String2 && RtlUpperChar( *String1 ) == RtlUpperChar( *String2 ) ) {

            String1++, String2++;
        }
        return RtlUpperChar( *String1 ) - RtlUpperChar( *String2 );
    }
}

LONG
RtlCompareAnsiStringLength(
    _In_ PCSTR   String1,
    _In_ PCSTR   String2,
    _In_ BOOLEAN CaseInsensitive,
    _In_ ULONG64 Length
)
{
    if ( !CaseInsensitive ) {
        while ( --Length && *String1 && *String2 && *String1 == *String2 ) {

            String1++, String2++;
        }
        return *String1 - *String2;
    }
    else {
        while ( --Length && *String1 && *String2 && RtlUpperChar( *String1 ) == RtlUpperChar( *String2 ) ) {

            String1++, String2++;
        }
        return RtlUpperChar( *String1 ) - RtlUpperChar( *String2 );
    }
}

/* Retrieves string length. */
ULONG lstrlenW(
    _In_ PCWSTR String // String.
) {
    ULONG Length = 0;

    while ( String[ Length++ ] != 0 );
    return Length - 1; // Return the length excluding a null terminator.
}

/* Compares 2 strings. (Case-sensitive) */
LONG lstrcmpW(
    _In_ PCWSTR String1,    // String 1.
    _In_ PCWSTR String2     // String 2.
) {
    while ( *String1++ == *String2++ )
        if ( *String1 == 0 ) return 0;      // Return  0 if strings are the same.

    if ( *--String1 > *--String2 ) return 1;    // Return  1 if String1 is bigger.
    else return -1;                         // Return -1 if String2 is bigger.
}

/* Compares 2 strings. */
LONG lstrcmpiW(
    _In_ PCWSTR String1,    // String 1.
    _In_ PCWSTR String2     // String 2.
) {
    while ( RtlUpperChar( *String1 ) == RtlUpperChar( *String2 ) )
        if ( *String1++ == 0 || *String2++ == 0 ) return 0; // Return  0 if strings are the same.

    if ( RtlUpperChar( *String1 ) > RtlUpperChar( *String2 ) ) return 1;    // Return  1 if String1 is bigger.
    else return -1;                                         // Return -1 if String2 is bigger.
}

/* Compares 2 string by length. (Case-sensitive) */
LONG lstrncmpW(
    _In_ PCWSTR String1,    // String 1.
    _In_ PCWSTR String2,    // String 2.
    _In_ ULONG  Characters  // Characters to compare.
) {
    while ( *String1++ == *String2++ && --Characters )
        if ( *String1 == 0 ) return 0; // Return 0 if characters are the same.

    return Characters; // Return the amount of different characters.
}

/* Compares 2 strings by length. */
LONG lstrncmpiW(
    _In_ PCWSTR String1,    // String 1.
    _In_ PCWSTR String2,    // String 2.
    _In_ ULONG  Characters  // Characters to compare.
) {
    while ( RtlUpperChar( *String1 ) == RtlUpperChar( *String2 ) && --Characters )
        if ( *String1++ == 0 || *String2++ == 0 ) return 0; // Return 0 if characters are the same.

    return Characters; // Return the amount of different characters.
}

/* Copies contents from one string to another. */
VOID lstrcpyW(
    _Inout_ PWSTR   DestinationString,  // Destination string.
    __in    PWSTR   SourceString        // Source string.
) {
    while ( *SourceString != 0 )
        *DestinationString++ = *SourceString++;
    *DestinationString = 0;
}

/* Copies a certain amount of characters from one string to another. */
VOID lstrncpyW(
    _Inout_ PWSTR DestinationString,    // Destination string.
    __in    PWSTR SourceString,         // Source string.
    __in    ULONG Characters            // Characters to copy.
) {
    while ( *SourceString != 0 && Characters-- )
        *DestinationString++ = *SourceString++;

    *DestinationString = 0;
}

/* Concatenates 2 strings. */
VOID lstrcatW(
    _In_ PWSTR  DestinationString,  // Destination string.
    _In_ PCWSTR SourceString        // Source string.
) {
    while ( *DestinationString != 0 ) DestinationString++;
    while ( *SourceString != 0 ) *DestinationString++ = *SourceString++;

    *DestinationString = 0;
}

/* Concatenates amount of characters of the second string with the first one. */
VOID lstrncatW(
    _In_ PWSTR  DestinationString,  // Destination string.
    _In_ PCWSTR SourceString,       // Source string.
    _In_ ULONG  Characters          // Characters to concatenate.
) {
    while ( *++DestinationString != 0 );
    while ( *SourceString != 0 && Characters-- ) *DestinationString++ = *SourceString++;

    *DestinationString = 0;
}

/* Locates the first occurrence of character in the string. (Case-sensitive) */
PWSTR lstrchrW(
    _In_ PWSTR String,      // String.
    _In_ WCHAR Character    // Character.
) {
    while ( *String++ != 0 )
        if ( *String == Character ) return String; // Return pointer to the first occurence.

    return NULL; // Return NULL pointer if no matches.
}

/* Locates the first occurrence of character in the string. */
PWSTR lstrchriW(
    _In_ PWSTR String,      // String.
    _In_ WCHAR Character    // Character.
) {
    while ( *String++ != 0 )
        if ( RtlUpperChar( *String ) == RtlUpperChar( Character ) ) return String; // Return pointer to the first occurence.

    return NULL; // Return NULL pointer if no matches.
}

/* Locates the first occurence of string in the string. (Case-sensitive) */
PWSTR lstrstrW(
    _In_ PWSTR String,      // String.
    _In_ PWSTR Substring    // Substring.
) {
    do {
        PWSTR SubstringAddress = Substring;

        while ( *String == *SubstringAddress )
            if ( String++, *++SubstringAddress == 0 ) return String - ( SubstringAddress - Substring ); // Return pointer to the first occurence.
    } while ( *String++ != 0 );

    return NULL; // Return NULL pointer if no matches.
}

/* Locates the first occurence of string in the string. */
PWSTR lstrstriW(
    _In_ PWSTR String,      // String.
    _In_ PWSTR Substring    // Substring.
) {
    do {
        PWSTR SubstringAddress = Substring;

        while ( RtlUpperChar( *String ) == RtlUpperChar( *SubstringAddress ) ) {
            if ( *SubstringAddress++ == 0 ) return String - SubstringAddress + Substring; // Return pointer to the first occurence.
            String++;
        }
    } while ( *String != 0 );

    return NULL; // Return NULL pointer if no matches.
}

wchar_t* wcsncpy( wchar_t* destination, const wchar_t* source, size_t num ) {
    wchar_t* o;

    o = destination;

    while ( num && *source ) {

        *destination++ = *source++;
        num--;
    }

    while ( num ) {

        *destination++ = 0;
        num--;
    }

    return o;
}
