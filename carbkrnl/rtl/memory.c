


#include <carbsup.h>

#pragma function(memset)
void*
memset(
    void*   dest,
    int     value,
    size_t  length
)
{
    char* m1;
    size_t n;

    m1 = dest;
    n = length;

    while ( n ) {
        *m1 = ( char )value;
        m1++;
        n--;
    }
    return dest;
}


VOID
RtlZeroMemory(
    _In_ PVOID Destination,
    _In_ ULONG Length
)
{
#if 0
    __m128i* u1;
    __m128i u2 = { 0 };

    u1 = Destination;

    while ( Length >= 16 ) {
        _mm_store_si128( u1, _mm_loadu_si128( &u2 ) );
        u1++, Length -= 16;
    }

    while ( Length-- ) {
        *( ( unsigned char* )u1 ) = 0;
        ( ( unsigned char* )u1 )++;
    }
#endif // put intrin shit elsewhere

    PUCHAR dst = Destination;
    while ( Length-- ) {
        *dst++ = 0;
    }
}

VOID
RtlCopyMemory(
    _In_ PVOID   Destination,
    _In_ PVOID   Source,
    _In_ ULONG64 Length
)
{
#if 0
    __m128i* u1;
    __m128i* u2;

    u1 = Destination;
    u2 = Source;

    while ( Length >= 16 ) {
        _mm_storeu_si128( u1, _mm_loadu_si128( u2 ) );
        u1++, u2++, Length -= 16;
    }

    while ( Length-- ) {
        *( ( unsigned char* )u1 ) = *( ( unsigned char* )u2 );
        ( ( unsigned char* )u1 )++, ( ( unsigned char* )u2 )++;
    }
#endif
#if 0
    PUCHAR dst = Destination;
    PUCHAR src = Source;
    while ( Length-- ) {
        *dst++ = *src++;
    }
#endif
    __movsb( Destination, Source, Length );
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

    m1 = Destination;
    m2 = Source;

    if ( m1 == m2 ) {
        return;
    }

    if ( m1 > m2 && m1 - m2 < Length ) {
        // <src......>
        //      <dst.....>
        for ( ULONG i = Length - 1; i > 0; i-- ) {
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

LONG
RtlCompareMemory(
    _In_ PVOID Memory1,
    _In_ PVOID Memory2,
    _In_ ULONG Length
)
{
    PUCHAR m1, m2;
    ULONG n;

    m1 = Memory1;
    m2 = Memory2;
    n = Length;

    while ( n ) {
        if ( *m1 != *m2 ) {

            return *m1 - *m2;
        }
        m1++, m2++;
        n--;
    }
    return 0;
}
