/*++

Module ObjectName:

	hpet.c

Abstract:

	High precision event timer.

--*/

#include <carbsup.h>
#include "hal.h"
#include "acpi.h"



typedef struct _HPET {
	SDT_HEADER Sdt;
	UCHAR HardwareRevision;
	UCHAR ComparatorCount : 5;
	UCHAR CounterSize : 1;
	UCHAR Reserved : 1;
	UCHAR LegacyReplacement : 1;
	USHORT PciVendorId;

	UCHAR AddressSpaceId; //0 sysmem, 1 sysio
	UCHAR RegisterBitWidth;
	UCHAR RegisterBitOffset;
	UCHAR Reserved0;
	ULONG64 Address;

	UCHAR HpetNumber;
	USHORT MinimumTick;
	UCHAR PageProtection;

} HPET, *PHPET;

VOID
HalHpetEnable(

	)
{

	HalAcpiFindSdt("HPET");


	/*
		at this point i realised it's shit.
	*/
	__halt();
}