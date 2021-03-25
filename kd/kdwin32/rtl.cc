


#include <intrin.h>
#include <windows.h>
#include "kd.h"

VOID
RtlCopyMemory(
    _In_ PVOID   Destination,
    _In_ PVOID   Source,
    _In_ ULONG64 Length
)
{
    __m128i* u1;
    __m128i* u2;
    unsigned char* v1;
    unsigned char* v2;

    u1 = ( __m128i* )Destination;
    u2 = ( __m128i* )Source;

    while ( Length >= 16 ) {
        _mm_storeu_si128( u1, _mm_loadu_si128( u2 ) );
        u1++, u2++, Length -= 16;
    }

    v1 = ( unsigned char* )u1;
    v2 = ( unsigned char* )u2;

    while ( Length-- ) {
        *v1 = *v2;
        v1++, v2++;
    }
}

VOID
RtlMoveMemory(
    _In_ PVOID Destination,
    _In_ PVOID Source,
    _In_ ULONG Length
)
{
    //https://opensource.apple.com/source/network_cmds/network_cmds-481.20.1/unbound/compat/memmove.c.auto.html
    PUCHAR m1, m2;

    m1 = ( PUCHAR )Destination;
    m2 = ( PUCHAR )Source;

    if ( m1 == m2 ) {
        return;
    }

    if ( m1 > m2 && m1 - m2 < Length ) {
        // <src......>
        //      <dst.....>
        for ( ULONG i = Length - 1; i >= 0; i-- ) {
            m1[ i ] = m2[ i ];
        }
        return;
    }

    if ( m2 > m1 && m2 - m1 < Length ) {
        //         <src....>
        //  <dst........>
        for ( ULONG i = 0; i < Length; i++ ) {
            m1[ i ] = m2[ i ];
        }
        return;
    }
    RtlCopyMemory( m1, m2, Length );
}
