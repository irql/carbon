


#include <carbsup.h>
#include "halp.h"
#include "../ke/ki.h"
#include "../io/iop.h"

EXTERN ULONG64 KxIntHandlerTable[ ];
EXTERN ULONG64 KiInterruptHandleBase[ ];

VOID
HalInitializeIdt(
    _Inout_ PKIDT_GATE     Table,
    _Out_   PKSEG_DESC_REG Idtr
)
{
    ULONG64 i = 0;

    for ( i = 0; i < 256; i++ ) {
        Table[ i ].OffsetLow = KxIntHandlerTable[ i ];
        Table[ i ].OffsetMid = KxIntHandlerTable[ i ] >> 16;
        Table[ i ].OffsetHigh = KxIntHandlerTable[ i ] >> 32;

        if ( i == 0x20 ) {

            Table[ i ].Ist = 1;
        }
        else if ( i < 0x20 && i != 0x2c && i != 0x29 ) {

            Table[ i ].Ist = 2;
        }
        else {

            Table[ i ].Ist = 0;
        }

        //Table[ i ].Ist = 1;
        //Table[ i ].Ist = i == 0x8;

        //Table[ i ].Ist = i == 0x20 ? 1 : 2;
        Table[ i ].CodeSelector = GDT_KERNEL_CODE64;
        Table[ i ].PrivilegeLevel = 0;
        Table[ i ].Present = 1;
        Table[ i ].Type = IDT_GATE_TYPE_INTERRUPT64;
    }

    Table[ 0x29 ].PrivilegeLevel = 3; // fast fail
    Table[ 0x2c ].PrivilegeLevel = 3; // assert fail
    Table[ 0x03 ].PrivilegeLevel = 3; // break point

    Idtr->Base = ( ULONG64 )Table;
    Idtr->Limit = sizeof( KIDT_GATE[ 256 ] ) - 1;
}

VOID
KiHardwareDispatch(
    _In_ PKTRAP_FRAME TrapFrame
)
{
    KINTERRUPT Interrupt;
    PIO_INTERRUPT IoInterrupt;
    KIRQL PreviousIrql;
    BOOLEAN InterruptingService;
    ULONG64 PreviousService;

    KeRaiseIrql( DISPATCH_LEVEL, &Interrupt.PreviousIrql );

    InterruptingService = KeQueryCurrentProcessor( )->InService;
    if ( InterruptingService ) {

        PreviousService = KeQueryCurrentProcessor( )->PreviousService;
    }

    KeQueryCurrentProcessor( )->InService = TRUE;
    KeQueryCurrentProcessor( )->PreviousService = TrapFrame->Interrupt;

    IoInterrupt = IopGetServiceHandler( ( ULONG )TrapFrame->Interrupt );

    while ( IoInterrupt != NULL ) {

        //
        // jon is bussin!
        //

        KeRaiseIrql( IoInterrupt->Irql, &PreviousIrql );

        Interrupt.TrapFrame = TrapFrame;
        if ( PsGetCurrentThread( ) != NULL ) {

            Interrupt.PreviousMode = ( PsGetCurrentThread( )->TrapFrame.SegCs & 1 ) == 0 ? KernelMode : UserMode;
        }

        if ( IoInterrupt->ServiceRoutine( &Interrupt,
                                          IoInterrupt->ServiceContext ) ) {
            KeLowerIrql( PreviousIrql );
            break;
        }

        KeLowerIrql( PreviousIrql );

        IoInterrupt = IoInterrupt->Link;
    }


    KeQueryCurrentProcessor( )->InService = InterruptingService;
    if ( InterruptingService ) {

        KeQueryCurrentProcessor( )->PreviousService = InterruptingService;
    }

    KeLowerIrql( Interrupt.PreviousIrql );
}

VOID
HalEoi(
    _In_ ULONG64 Vector
)
{
    Vector;
    HalLocalApicWrite( LAPIC_END_OF_INTERRUPT_REGISTER, 0 );
}
