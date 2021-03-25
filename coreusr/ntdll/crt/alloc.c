


#include <carbusr.h>
#include "../ntdll.h"

size_t _msize( void* ptr ) {

    return RtlAllocationSizeHeap( NtCurrentPeb( )->ProcessHeap, ptr );
}

void* malloc( size_t size ) {
    void* ptr;
    ptr = RtlAllocateHeap( NtCurrentPeb( )->ProcessHeap, size );;
    //RtlDebugPrint( L"malloc: %ull %d\n", ptr, size );
    return ptr;
}

void free( void* ptr ) {
    ptr;
    //RtlFreeHeap( NtCurrentPeb( )->ProcessHeap, ptr );
}

void* calloc( size_t num, size_t size ) {

    return memset( malloc( num * size ), 0, num * size );
}

void* realloc( void* ptr, size_t new_size ) {

    return memcpy( malloc( new_size ), ptr, _msize( ptr ) );
}
