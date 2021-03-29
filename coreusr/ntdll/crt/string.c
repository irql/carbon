


#include <carbusr.h>
#include "../ntdll.h"

//
// TODO: optimize all with SSE2
//

#pragma comment(linker, "/export:strcmp")
#pragma function(strcmp)
int strcmp( const char* str1, const char* str2 ) {
    while ( *str1 && *str2 && *str1 == *str2 )
        str1++, str2++;
    return *str1 - *str2;
}

int strncmp( const char* str1, const char* str2, size_t num ) {
    while ( num-- && *str1 && *str2 && *str1 == *str2 )
        str1++, str2++;
    return *str1 - *str2;
}

#pragma comment(linker, "/export:strlen")
#pragma function(strlen)
size_t strlen( const char* str1 ) {
    size_t i = 0;

    while ( str1[ i ] )
        i++;

    return i;
}

#pragma comment(linker, "/export:strcpy")
#pragma function(strcpy)
char* strcpy( char* destination, const char* source ) {
    char* temp = destination;

    while ( *source )
        *destination++ = *source++;
    *destination = 0;
    return temp;
}

#pragma comment(linker, "/export:strcat")
#pragma function(strcat)
char* strcat( char* destination, const char* source ) {
    char* temp = destination;

    while ( *destination )
        destination++;
    strcpy( destination, source );
    return temp;
}

char* strdup( char* string ) {

    return strcpy( malloc( strlen( string ) + 1 ), string );
}

long int strtol( const char* str, char** end, int base ) {
    int i;
    int sign;
    long int val;

    sign = 1;
    i = 0;
    val = 0;

    if ( str[ i ] == '-' ) {

        sign = -1;
        i++;
    }

    if ( str[ i ] == '+' ) {

        i++;
    }

    if ( base == 0 ) {

        if ( str[ i ] == '0' && tolower( str[ i + 1 ] ) == 'x' ) {

            base = 16;
            i += 2;
        }
        else {

            base = 10;
        }
    }
    else if ( base == 16 && str[ i ] == '0' && str[ i + 1 ] == 'x' ) {

        i += 2;
    }

    while ( str[ i ] != 0 && (
        ( str[ i ] >= '0' && str[ i ] <= '9' ) ||
        ( base > 10 && tolower( str[ i ] ) >= 'a' && tolower( str[ i ] ) <= 'f' ) )
        ) {

        if ( str[ i ] >= '0' && str[ i ] <= '9' ) {

            val = val * base + str[ i ] - '0';
        }
        else {

            val = val * base + str[ i ] - 'a' + 10;
        }

        i++;
    }

    val *= sign;

    if ( end ) {
        *end = ( char* )str + i;
    }

    return val;
}

const char* strrchr( const char* str, int character ) {
    const char* o;

    o = str;

    while ( *str )
        str++;

    while ( str != o ) {

        if ( *str == character ) {

            return str;
        }

        str--;
    }

    return NULL;
}

const char* strstr( const char *s1, const char *s2 ) {
    int i;

    while ( *s1 ) {

        for ( i = 0; s2[ i ]; i++ ) {

            if ( s1[ i ] != s2[ i ] ) {

                break;
            }
        }

        if ( s2[ i ] == 0 ) {

            return s1;
        }

        s1++;
    }

    return NULL;
}

char* strncpy( char* destination, const char* source, size_t num ) {
    char* o;

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

//
// memory
//

#pragma comment(linker, "/export:memcmp")
#pragma function(memcmp)
int memcmp( const void* ptr1, void* ptr2, size_t num ) {
    const unsigned char* m1 = ptr1;
    const unsigned char* m2 = ptr2;

    while ( num-- && *m1 == *m2 )
        m1++, m2++;
    return *m1 - *m2;
}

#pragma comment(linker, "/export:memcpy")
#pragma function(memcpy)
void* memcpy( void* destination, void* source, size_t num ) {
    const unsigned char* m1 = source;
    unsigned char* m2 = destination;

    while ( num-- )
        *m2++ = *m1++;
    return destination;
}

#pragma comment(linker, "/export:memset")
#pragma function(memset)
void* memset( void* mem, int v, size_t num ) {
    unsigned char* m2 = mem;

    while ( num-- )
        *m2++ = ( unsigned char )v;
    return mem;
}

#pragma comment(linker, "/export:memmove")
#pragma function(memmove)
void* memmove( void* destination, void* source, size_t num ) {
    unsigned char* m2 = source;
    unsigned char* m1 = destination;

    if ( m1 > m2 && ( unsigned )( m1 - m2 ) < num ) {

        for ( size_t i = num - 1; i > 0; i-- )
            m1[ i ] = m2[ i ];
    }
    else if ( m2 > m1 && ( unsigned )( m2 - m1 ) < num ) {

        for ( size_t i = 0; i < num; i++ )
            m1[ i ] = m2[ i ];
    }
    else {

        memcpy( m1, m2, num );
    }

    return destination;
}

#pragma comment(linker, "/export:memchr")
#pragma function(memchr)
void* memchr( void* ptr, int value, size_t num ) {
    unsigned char* m1;

    m1 = ptr;

    while ( num-- ) {

        if ( *m1 == ( unsigned char )value ) {

            return m1;
        }

        m1++;
    }

    return NULL;
}

//
// unicode
//

#pragma comment(linker, "/export:wcslen")
#pragma function( wcslen )
size_t wcslen( const wchar_t* string ) {
    size_t i = 0;

    while ( string[ i ] )
        i++;

    return i;
}

#pragma comment(linker, "/export:wcscpy")
#pragma function( wcscpy )
wchar_t* wcscpy( wchar_t* destination, const wchar_t* source ) {
    wchar_t* original;
    original = destination;

    while ( *source )
        *destination++ = *source++;

    *destination = 0;
    return original;
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

#pragma comment(linker, "/export:wcscat")
#pragma function( wcscat )
wchar_t* wcscat( wchar_t* destination, const wchar_t* source ) {
    wchar_t* temp = destination;

    while ( *destination )
        destination++;
    wcscpy( destination, source );
    return temp;
}

size_t mbstowcs( wchar_t* wcstr, const char* mbstr, size_t count ) {

    if ( wcstr ) {
        while ( count-- && *mbstr )
            *wcstr++ = *mbstr++;
        while ( count ) {
            *wcstr++ = 0;
            count--;
        }
        return 0;
    }
    else {
        return strlen( mbstr );
    }
}

wchar_t* wcsdup( wchar_t* string ) {

    return wcscpy( malloc( wcslen( string ) + sizeof( WCHAR ) ), string );
}
