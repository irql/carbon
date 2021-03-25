


#include <carbusr.h>
#include "../ntdll.h"

inline void qswap( void* a, void* b, size_t size ) {
    unsigned char* m1;
    unsigned char* m2;
    unsigned char  temp;

    m1 = a;
    m2 = b;

    while ( size-- ) {
        temp = *m1;
        *m1 = *m2;
        *m2 = temp;
        m1++, m2++;
    }
}

int qpart( void* base, size_t num, size_t size, int( *compare )( const void*, const void* ) ) {

    void* pivot;
    int low;
    int current;

    pivot = ( char* )base + ( num - 1 ) * size;
    low = 0;

    for ( current = 0; current < ( int )( num - 1 ); current++ ) {

        if ( compare( ( char* )base + current * size, pivot ) < 0 ) {

            qswap( ( char* )base + current * size, ( char* )base + low * size, size );
            low++;
        }
    }

    qswap( ( char* )base + low * size, ( char* )base + ( num - 1 ) * size, size );
    return low;
}

int compare_arr( const int* a, const int* b ) {
    if ( *a < *b ) return -1;
    if ( *a > *b ) return 1;
    return 0;
}

void qsort( void* base, size_t num, size_t size, int( *compare )( const void*, const void* ) ) {
    int part;

    if ( num > 1 ) {

        part = qpart( base, num, size, compare );
        if ( part > 0 ) {

            qsort( base, part - 1, size, compare );
        }
        qsort( ( char* )base + ( part + 1 ) * size, num - part - 1, size, compare );
    }
}
