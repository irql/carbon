/*++

Module ObjectName:

	mem.c

Abstract:

	Defines run time library memory routines for the system.

--*/


#include <carbsup.h>


void* _memset( void* m1, unsigned char v, __int64 n ) {

	unsigned char* u1 = m1;

	while ( n-- )
		*u1++ = ( unsigned char )v;

	return m1;
	//_mm_unpacklo_epi8();
}

void* _memcpy( void* dst, void* src, int n ) {
	__m128i* u1 = ( __m128i* )dst;
	__m128i* u2 = ( __m128i* )src;

	while ( n >= 16 ) {
		_mm_storeu_si128( u1, _mm_loadu_si128( u2 ) );
		u1++;
		u2++;
		n -= 16;
	}

	while ( n-- ) {
		*( ( unsigned char* )u1 ) = *( ( unsigned char* )u2 );
#pragma warning(disable:4213)
		( ( unsigned char* )u1 )++;
		( ( unsigned char* )u2 )++;
#pragma warning(default:4213)
	}

	return dst;
}

unsigned char _memcmp( void* m1, void* m2, int n ) {

	unsigned char* u1 = ( unsigned char* )m1;
	unsigned char* u2 = ( unsigned char* )m2;

	while ( n-- && ( *u1++ == *u2++ ) )
		;

	return *--u1 - *--u2;
}
