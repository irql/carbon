/*++

Module ObjectName:

	pic.c

Abstract:

	Implements programmable interrupt controller 8259A procedures.

--*/


#include <carbsup.h>
#include "hal.h"

VOID
HalPic8259aSetIrqMasks(
	__in UCHAR MasterMask,
	__in UCHAR SlaveMask
) {
	__outbyte(PIC_MASTER_IOPORT_DATA, MasterMask);
	__outbyte(PIC_SLAVE_IOPORT_DATA, SlaveMask);
}

VOID
HalPic8259aGetIrqMasks(
	__out PUCHAR MasterMask,
	__out PUCHAR SlaveMask
) {
	*MasterMask = __inbyte(PIC_MASTER_IOPORT_DATA);
	*SlaveMask = __inbyte(PIC_SLAVE_IOPORT_DATA);
}

VOID
HalPic8259aRemapVectorOffsets(
	__in UCHAR MasterOffset,
	__in UCHAR SlaveOffset
) {
	UCHAR MasterMask, SlaveMask;

	HalPic8259aGetIrqMasks(&MasterMask, &SlaveMask);

	__outbyte(PIC_MASTER_IOPORT_COMMAND, ICW1_INIT | ICW1_ICW4);     //icw1 master init
	__outbyte(PIC_SLAVE_IOPORT_COMMAND, ICW1_INIT | ICW1_ICW4);      //icw1 slave init

	__outbyte(PIC_MASTER_IOPORT_DATA, MasterOffset);                 //icw2 master vector offset
	__outbyte(PIC_SLAVE_IOPORT_DATA, SlaveOffset);                   //icw2 slave vector offset

	__outbyte(PIC_MASTER_IOPORT_DATA, 0x04);                         //icw3 tell master there is a slave pic at irq2
	__outbyte(PIC_SLAVE_IOPORT_DATA, 0x02);                          //icw3 tell slave its cascade identity

	__outbyte(PIC_MASTER_IOPORT_DATA, ICW4_8086);                    //icw4
	__outbyte(PIC_SLAVE_IOPORT_DATA, ICW4_8086);                     //icw4

	HalPic8259aSetIrqMasks(MasterMask, SlaveMask);
}


