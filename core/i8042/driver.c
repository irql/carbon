


#include <carbsup.h>
#include "i8042.h"

EXTERN ULONG64 I8042MouseType;

NTSTATUS
DriverLoad(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    DriverObject;
    ///*
    KIRQL PreviousIrql;
    KAPIC_REDIRECT Redirect;
    UCHAR Status;
    PIO_INTERRUPT InterruptObject;
    OBJECT_ATTRIBUTES Interrupt = { 0 };
    //*/
    //
    // This driver is not integrated with any kind of acpi
    // so it hardcodes interrupt request vectors, which should eventually
    // be changed to enumerate acpi hardware and get the vector.
    //
    // I also plan on implementing vmbackdoor drivers, so there should be some
    // kind of method to acquire ownership of hardware for drivers, so that multiple
    // drivers are not trying to drive the same piece.
    //

    //
    // notes on this being the first irq-using driver: implement
    // these fucking apis better.
    //
    // dpcs are kinda wacky, need a better method for doing some task at a lower
    // priority, due to the rules of irqs (no spinlocks lol) 
    //

    //
    // Set up the i8042 keyboard interrupt handler
    //
#if 1
    IoConnectInterrupt( &InterruptObject,
        ( KSERVICE_ROUTINE )I8042KeyboardInterrupt,
                        NULL,
                        0x40,
                        4,
                        &Interrupt );
    ObDereferenceObject( InterruptObject );

    Redirect.Lower = 0;
    Redirect.Upper = 0;

    Redirect.InterruptVector = 0x40;
    Redirect.DeliveryMode = DeliveryModeEdge;
    Redirect.DestinationMode = DestinationModePhysical;
    Redirect.Destination = 0;

    HalApicRedirectIrq( 1, &Redirect );

    IoConnectInterrupt( &InterruptObject,
        ( KSERVICE_ROUTINE )I8042MouseInterrupt,
                        NULL,
                        0x41,
                        4,
                        &Interrupt );
    ObDereferenceObject( InterruptObject );

    Redirect.Lower = 0;
    Redirect.Upper = 0;

    Redirect.InterruptVector = 0x41;
    Redirect.DeliveryMode = DeliveryModeEdge;
    Redirect.DestinationMode = DestinationModePhysical;
    Redirect.Destination = 0;

    HalApicRedirectIrq( 12, &Redirect );

    KeRaiseIrql( DISPATCH_LEVEL, &PreviousIrql );

    __outbyte( I8042_REG_STATUS, 0xA8 );

    I8042MouseWait( 1 );
    __outbyte( I8042_REG_STATUS, 0x20 );

    I8042MouseWait( 0 );
    Status = ( __inbyte( I8042_REG_OUTPUT ) | ( 1 << 1 ) ) & ~( 1 << 5 );

    __inbyte( I8042_REG_OUTPUT );//Bochs sends 0xD8

    I8042MouseWait( 1 );
    __outbyte( I8042_REG_STATUS, 0x60 );

    I8042MouseWait( 1 );
    __outbyte( I8042_REG_OUTPUT, Status );
    I8042MouseRead( );//might generate ACK

    I8042MouseWrite( I8042_CMD_SET_DEFAULTS );
    I8042MouseRead( );


    I8042MouseWrite( I8042_CMD_GET_MOUSE_ID );
    I8042MouseRead( );

    I8042MouseType = I8042MouseRead( );

    if ( I8042MouseType != I8042MouseScroll ) {

        I8042MouseSetSampleRate( 200 );
        I8042MouseSetSampleRate( 100 );
        I8042MouseSetSampleRate( 80 );
        I8042MouseWrite( I8042_CMD_GET_MOUSE_ID );
        I8042MouseRead( );

        I8042MouseType = I8042MouseRead( );

        //RtlDebugPrint( L"Cool Mouse: %d\n", I8042MouseType );

        if ( I8042MouseType != I8042MouseScroll ) {

            // so qemu sends 0x41 for mousetype?
            // ignoring it is fine though.

            I8042MouseType = I8042MouseScroll;//I8042MouseStandard;
        }
    }

    I8042MouseWrite( I8042_CMD_ENABLE_PACKET_STREAMING );
    I8042MouseRead( );
    KeLowerIrql( PreviousIrql );
#endif
    return STATUS_SUCCESS;
}
