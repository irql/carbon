


#include <carbsup.h>
#include "../hal/halp.h"
#include "ki.h"

#define CMOS_ADDR               0x70
#define CMOS_DATA               0x71

#define RTC_REGISTER_A          0x0A
#define RTC_REGISTER_B          0x0B
#define RTC_REGISTER_C          0x0C
#define RTC_REGISTER_D          0x0D

#define RTC_REGISTER_SECOND     0x00
#define RTC_REGISTER_MINUTE     0x02
#define RTC_REGISTER_HOUR       0x03
#define RTC_REGISTER_DAY        0x07
#define RTC_REGISTER_MONTH      0x08
#define RTC_REGISTER_YEAR       0x09

KSYSTEM_TIME KiGlobalTime = { 0 };

FORCEINLINE
UCHAR
KiReadRTC(
    _In_ UCHAR Register
)
{
    __outbyte( CMOS_ADDR, Register );
    return __inbyte( CMOS_DATA );
}

FORCEINLINE
VOID
KiWriteRTC(
    _In_ UCHAR Register,
    _In_ UCHAR Value
)
{
    __outbyte( CMOS_ADDR, Register );
    __outbyte( CMOS_DATA, Value );
}

FORCEINLINE
UCHAR
KiBcdToBin(
    _In_ UCHAR Bcd
)
{
    return ( ( Bcd / 16 ) * 10 ) + ( Bcd & 0x0F );
}

BOOLEAN
KiUpdateClock(
    _In_ PKINTERRUPT Interrupt
)
{
    Interrupt;

    if ( KiReadRTC( RTC_REGISTER_C ) & ( 1 << 4 ) ) {
        if ( KiReadRTC( RTC_REGISTER_B ) & 0x4 ) {
            KiGlobalTime.Second = KiReadRTC( RTC_REGISTER_SECOND );
            KiGlobalTime.Minute = KiReadRTC( RTC_REGISTER_MINUTE );
            KiGlobalTime.Hour = KiReadRTC( RTC_REGISTER_HOUR );
            KiGlobalTime.Day = KiReadRTC( RTC_REGISTER_DAY );
            KiGlobalTime.Month = KiReadRTC( RTC_REGISTER_MONTH );
            KiGlobalTime.Year = KiReadRTC( RTC_REGISTER_YEAR );
        }
        else {

            KiGlobalTime.Second = KiBcdToBin( KiReadRTC( RTC_REGISTER_SECOND ) );
            KiGlobalTime.Minute = KiBcdToBin( KiReadRTC( RTC_REGISTER_MINUTE ) );
            KiGlobalTime.Hour = KiBcdToBin( KiReadRTC( RTC_REGISTER_HOUR ) );
            KiGlobalTime.Day = KiBcdToBin( KiReadRTC( RTC_REGISTER_DAY ) );
            KiGlobalTime.Month = KiBcdToBin( KiReadRTC( RTC_REGISTER_MONTH ) );
            KiGlobalTime.Year = KiBcdToBin( KiReadRTC( RTC_REGISTER_YEAR ) );

        }
    }

    return TRUE;
}

VOID
KeQuerySystemTime(
    _Out_ PKSYSTEM_TIME ClockTime
)
{
    *ClockTime = KiGlobalTime;
}

NTSTATUS
NtQuerySystemClock(
    _In_ PKSYSTEM_TIME ClockTime
)
{
    __try {

        *ClockTime = KiGlobalTime;
        return STATUS_SUCCESS;
    }
    __except ( EXCEPTION_EXECUTE_HANDLER ) {

        return STATUS_ACCESS_VIOLATION;
    }
}

VOID
KeInitializeKernelClock(

)
{
    KAPIC_REDIRECT Redirect;
    PIO_INTERRUPT InterruptObject;
    OBJECT_ATTRIBUTES Interrupt = { 0 };
    KIRQL PreviousIrql;

    IoConnectInterrupt( &InterruptObject,
        ( KSERVICE_ROUTINE )KiUpdateClock,
                        NULL,
                        0x80,
                        8,
                        &Interrupt );
    ObDereferenceObject( InterruptObject );

    Redirect.Lower = 0;
    Redirect.Upper = 0;

    Redirect.InterruptVector = 0x80;
    Redirect.DeliveryMode = DeliveryModeEdge;
    Redirect.DestinationMode = DestinationModePhysical;
    Redirect.Destination = 0;

    HalApicRedirectIrq( 8, &Redirect );

    KeRaiseIrql( IPI_LEVEL, &PreviousIrql );

    // this doesn't even matter we use update-ended, not periodic
    KiWriteRTC( RTC_REGISTER_A, ( KiReadRTC( RTC_REGISTER_A ) & 0x90 ) | ( 6 ) | ( 1 << 5 ) );
    KiWriteRTC( RTC_REGISTER_B, KiReadRTC( RTC_REGISTER_B ) | ( 1 << 4 ) | ( 1 << 1 ) );
    KiReadRTC( RTC_REGISTER_C );

    KeLowerIrql( PreviousIrql );

}
