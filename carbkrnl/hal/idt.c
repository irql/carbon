


#include <carbsup.h>
#include "halp.h"
#include "../ke/ki.h"
#include "../io/iop.h"

EXTERN ULONG64 KxIntHandlerTable[ ];
EXTERN ULONG64 KiInterruptHandleBase[ ];

VOID
HalCreateInterrupt(
    _Inout_ PKIDT_GATE         Table,
    _Out_   PKDESCRIPTOR_TABLE Idtr
)
{
    ULONG64 i = 0;

    /*
    Because IA-32 architecture tasks are not re-entrant, an interrupt-handler task must disable
    interrupts between the time it completes handling the interrupt and the time it executes the IRET
    instruction. This action prevents another interrupt from occurring while the interrupt task’s TSS is
    still marked busy, which would cause a general-protection (#GP) exception.
    */

    for ( i = 0; i < 256; i++ ) {
        Table[ i ].OffsetLow = KxIntHandlerTable[ i ];
        Table[ i ].OffsetMid = KxIntHandlerTable[ i ] >> 16;
        Table[ i ].OffsetHigh = KxIntHandlerTable[ i ] >> 32;
        Table[ i ].Type = SYSTEM_SEGMENT_TYPE_INTERRUPT_GATE;

        //
        // wipet — Today at 15:51
        // the 2 last checks are useless
        // just noticed that
        //

        if ( i == 0x20 ) {

            Table[ i ].Ist = 1;
        }
        else if ( i == 0x8 ) {

            Table[ i ].Ist = 3;
        }
        else if ( i > 0x20 && i != 0x29 && i != 0x2c ) {

            Table[ i ].Ist = 2;
        }
        else {

            Table[ i ].Type = SYSTEM_SEGMENT_TYPE_TRAP_GATE;
            Table[ i ].Ist = 0;
        }

        Table[ i ].SegmentSelector = GDT_KERNEL_CODE64;
        Table[ i ].PrivilegeLevel = 0;
        Table[ i ].Present = 1;
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
