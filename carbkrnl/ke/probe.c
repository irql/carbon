


#include <carbsup.h>

#pragma optimize( "", off )
VOID
KeProbeForRead(
    _In_ PVOID   Address,
    _In_ ULONG64 Length
)
{
    // pretty much a template, dont both with a lot 
    ULONG64 UserAddress = ( ULONG64 )Address;
    VOLATILE CHAR Temp;

    if ( UserAddress >= 0x800000000000 ||
         UserAddress + Length >= 0x800000000000 ) {

        RtlRaiseException( 0 );
    }

    __try {

        while ( Length-- ) {
            Temp = *( ( volatile char* )Address + Length );
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER ) {

        RtlRaiseException( 0 );
    }
}
#pragma optimize( "", on )