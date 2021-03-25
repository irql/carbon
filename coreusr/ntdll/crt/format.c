


#include <carbusr.h>

//
// brutal, pasted from rtl/string.c
// add length arg.
//

wchar_t* wcsrev( wchar_t* String ) {
    ULONG i, j;
    WCHAR Temp;

    i = 0;
    j = ( ULONG )wcslen( String ) - 1;

    for ( ; i < j; i++, j-- ) {

        Temp = String[ i ];
        String[ i ] = String[ j ];
        String[ j ] = Temp;
    }

    return String;
}

void itow( int Value, wchar_t* Buffer, int Base ) {
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

    wcsrev( Buffer );
}

int wtoi( const wchar_t* Buffer ) {
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
          ( ( Base > 10 && tolower( Buffer[ Index ] ) >= 'a' && tolower( Buffer[ Index ] <= 'f' ) ) ) ); Index++ ) {//'a' + base - 10
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

char* strrev( char* String ) {
    ULONG i, j;
    CHAR Temp;

    i = 0;
    j = ( ULONG )strlen( String ) - 1;

    for ( ; i < j; i++, j-- ) {

        Temp = String[ i ];
        String[ i ] = String[ j ];
        String[ j ] = Temp;
    }

    return String;
}

void itoa( int Value, char* Buffer, int Base ) {
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
        Buffer[ Index++ ] = ( Temp > 9 ) ? ( CHAR )( Temp - 10 ) + 'a' : ( CHAR )( Temp + '0' );
        Value /= Base;
    }

    if ( Negative ) {
        Buffer[ Index++ ] = '-';
    }

    Buffer[ Index ] = 0;

    strrev( Buffer );
}

int atoi( const char* Buffer ) {
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
          ( ( Base > 10 && tolower( Buffer[ Index ] ) >= 'a' && tolower( Buffer[ Index ] <= 'f' ) ) ) ); Index++ ) {//'a' + base - 10
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


int vswprintf( wchar_t* Buffer, size_t Length, const wchar_t* Format, va_list List ) {

    Length;
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
            ZeroPad = wtoi( Format );
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

                itow( Lo32, LoBuf, 16 );
                itow( Hi32, HiBuf, 16 );
                LoPad = 8 - ( ULONG )wcslen( LoBuf );
                if ( Hi32 == 0 ) {
                    LoPad = 0;
                }

                HiPad = ZeroPad - ( ( ULONG32 )wcslen( LoBuf ) + LoPad );
                if ( Hi32 != 0 ) {
                    HiPad -= ( ULONG )wcslen( HiBuf );
                }

                for ( unsigned char i = 0; i < HiPad; i++, Index++ ) {
                    Buffer[ Index ] = '0';
                }

                for ( unsigned char i = 0; HiBuf[ i ] && Hi32 != 0; i++, Index++ ) {
                    Buffer[ Index ] = CodeUpper ? toupper( HiBuf[ i ] ) : HiBuf[ i ];
                }

                for ( unsigned char i = 0; i < LoPad; i++, Index++ ) {
                    Buffer[ Index ] = '0';
                }

                for ( unsigned char i = 0; LoBuf[ i ]; i++, Index++ ) {
                    Buffer[ Index ] = CodeUpper ? toupper( LoBuf[ i ] ) : LoBuf[ i ];
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

                itow( Lo32, LoBuf, 16 );
                LoPad = ZeroPad - ( LONG32 )wcslen( LoBuf );

                for ( int i = 0; LoPad > 0 && i < LoPad; i++, Index++ ) {
                    Buffer[ Index ] = '0';
                }

                for ( int i = 0; LoBuf[ i ]; i++, Index++ ) {
                    Buffer[ Index ] = CodeUpper ? toupper( LoBuf[ i ] ) : LoBuf[ i ];
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

            itow( Lo32, LoBuf, 10 );
            LoPad = ZeroPad - ( LONG32 )wcslen( LoBuf );

            for ( unsigned char i = 0; i < LoPad; i++, Index++ ) {
                Buffer[ Index ] = '0';
            }

            for ( unsigned char i = 0; LoBuf[ i ]; i++, Index++ ) {
                Buffer[ Index ] = CodeUpper ? toupper( LoBuf[ i ] ) : LoBuf[ i ];
            }

            Format++;
            break;
        default:;
            break;
        }

    }

    Buffer[ Index ] = 0;
    return 0;
}

int swprintf( wchar_t* buffer, const wchar_t* format, ... ) {
    int r;
    va_list args;
    __crt_va_start( args, format );
    r = vswprintf( buffer, 0, format, args );
    __crt_va_end( args );
    return r;
}

int vsprintf( char* Buffer, size_t Length, const char* Format, va_list List ) {

    Length;
    ULONG Index;
    BOOLEAN CodeHash;
    BOOLEAN CodePad;
    BOOLEAN CodeUpper;

    ULONG64 ArgULL64;
    ULONG32 Lo32;
    ULONG32 Hi32;
    CHAR LoBuf[ 16 ];
    CHAR HiBuf[ 16 ];
    LONG32 LoPad;
    LONG32 HiPad;
    LONG32 ZeroPad;
    PWCHAR ArgWS;
    WCHAR ArgWC;
    PCHAR ArgAS;
    CHAR ArgAC;

    Index = 0;

    RtlDebugPrint( L"Sprintf: %as\n", Buffer );

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
            ZeroPad = atoi( Format );
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

            if ( *Format == 's' ) {
                ArgWS = __crt_va_arg( List, PWCHAR );

                if ( ArgWS == NULL ) {
                    ArgWS = L"(null)";
                }
                else if ( *ArgWS == 0 ) {
                    ArgWS = L"(zero)";
                }

                for ( ULONG32 i = 0; ArgWS[ i ]; i++, Index++ ) {
                    Buffer[ Index ] = ( char )ArgWS[ i ];
                }

                Format++;
            }
            else if ( *Format == 'c' ) {
                ArgWC = __crt_va_arg( List, WCHAR );

                if ( ArgWC == 0 ) {
                    ArgWS = L"(zero)";
                    for ( ULONG32 i = 0; ArgWS[ i ]; i++, Index++ ) {
                        Buffer[ Index ] = ( CHAR )ArgWS[ i ];
                    }
                }
                else {
                    Buffer[ Index++ ] = ( CHAR )ArgWC;
                }
                Format++;
            }

            break;
        case 'a':;
            Format++;
        case 's':;
        case 'c':;

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

                itoa( Lo32, LoBuf, 16 );
                itoa( Hi32, HiBuf, 16 );
                LoPad = 8 - ( ULONG )strlen( LoBuf );
                if ( Hi32 == 0 ) {
                    LoPad = 0;
                }

                HiPad = ZeroPad - ( ( ULONG32 )strlen( LoBuf ) + LoPad );
                if ( Hi32 != 0 ) {
                    HiPad -= ( ULONG )strlen( HiBuf );
                }

                for ( unsigned char i = 0; i < HiPad; i++, Index++ ) {
                    Buffer[ Index ] = '0';
                }

                for ( unsigned char i = 0; HiBuf[ i ] && Hi32 != 0; i++, Index++ ) {
                    Buffer[ Index ] = CodeUpper ? toupper( HiBuf[ i ] ) : HiBuf[ i ];
                }

                for ( unsigned char i = 0; i < LoPad; i++, Index++ ) {
                    Buffer[ Index ] = '0';
                }

                for ( unsigned char i = 0; LoBuf[ i ]; i++, Index++ ) {
                    Buffer[ Index ] = CodeUpper ? toupper( LoBuf[ i ] ) : LoBuf[ i ];
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

                itoa( Lo32, LoBuf, 16 );
                LoPad = ZeroPad - ( LONG32 )strlen( LoBuf );

                for ( int i = 0; LoPad > 0 && i < LoPad; i++, Index++ ) {
                    Buffer[ Index ] = '0';
                }

                for ( int i = 0; LoBuf[ i ]; i++, Index++ ) {
                    Buffer[ Index ] = CodeUpper ? toupper( LoBuf[ i ] ) : LoBuf[ i ];
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

            itoa( Lo32, LoBuf, 10 );
            LoPad = ZeroPad - ( LONG32 )strlen( LoBuf );

            for ( unsigned char i = 0; i < LoPad; i++, Index++ ) {
                Buffer[ Index ] = '0';
            }

            for ( unsigned char i = 0; LoBuf[ i ]; i++, Index++ ) {
                Buffer[ Index ] = CodeUpper ? toupper( LoBuf[ i ] ) : LoBuf[ i ];
            }

            Format++;
            break;
        default:;
            break;
        }

    }

    Buffer[ Index ] = 0;
    return 0;
}

int sprintf( char* buffer, const char* format, ... ) {
    int r;
    va_list args;
    __crt_va_start( args, format );
    r = vsprintf( buffer, 0, format, args );
    __crt_va_end( args );
    return r;
}
