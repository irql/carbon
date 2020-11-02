/*++

Module ObjectName:

	pci.c

Abstract:

	Defines API's for enumerating the PCI devices and reading
	base address registers and PCI configuration data.

--*/

#include <carbsup.h>
#include "hal.h"

#define PCI_GET_ADDRESS(device, offset) ((ULONG32)((((ULONG32)device->Bus)<<16)|(((ULONG32)device->Device)<<11)|(((ULONG32)device->Function)<<8)|(((UCHAR)offset)&(~3))|PCI_ENABLE_BIT))

PCI_DEVICE_LIST HalPciDeviceList;

UCHAR
HalPciRead8(
	__in PPCI_DEVICE Device,
	__in ULONG Offset
)
{
	__outdword( PCI_CONFIG_ADDRESS, PCI_GET_ADDRESS( Device, Offset ) );
	return __inbyte( PCI_CONFIG_DATA + ( Offset & 3 ) );
}

USHORT
HalPciRead16(
	__in PPCI_DEVICE Device,
	__in ULONG Offset
)
{
	__outdword( PCI_CONFIG_ADDRESS, PCI_GET_ADDRESS( Device, Offset ) );
	return __inword( PCI_CONFIG_DATA + ( Offset & 2 ) );
}

ULONG32
HalPciRead32(
	__in PPCI_DEVICE Device,
	__in ULONG Offset
)
{
	__outdword( PCI_CONFIG_ADDRESS, PCI_GET_ADDRESS( Device, Offset ) );
	return __indword( PCI_CONFIG_DATA );
}

VOID
HalPciWrite8(
	__in PPCI_DEVICE Device,
	__in ULONG Offset,
	__in UCHAR Value
)
{
	__outdword( PCI_CONFIG_ADDRESS, PCI_GET_ADDRESS( Device, Offset ) );
	__outbyte( PCI_CONFIG_DATA + ( Offset & 3 ), Value );
}

VOID
HalPciWrite16(
	__in PPCI_DEVICE Device,
	__in ULONG Offset,
	__in USHORT Value
)
{
	__outdword( PCI_CONFIG_ADDRESS, PCI_GET_ADDRESS( Device, Offset ) );
	__outword( PCI_CONFIG_DATA + ( Offset & 2 ), Value );
}

VOID
HalPciWrite32(
	__in PPCI_DEVICE Device,
	__in ULONG Offset,
	__in ULONG Value
)
{
	__outdword( PCI_CONFIG_ADDRESS, PCI_GET_ADDRESS( Device, Offset ) );
	__outdword( PCI_CONFIG_DATA, Value );
}


VOID
HalPciReadBar(
	__in PPCI_DEVICE Device,
	__out PPCI_BASE_ADDRESS_REGISTER BaseAddressRegister,
	__in UCHAR Index
)
{
	ULONG32 Offset = FIELD_OFFSET( PCI_STANDARD_DEVICE, Bar ) + ( ( ULONG )Index * sizeof( ULONG ) );
	ULONG32 AddressLow = 0, AddressHigh = 0;
	ULONG32 MaskLow = 0, MaskHigh = 0;

	AddressLow = HalPciRead32( Device, Offset );
	HalPciWrite32( Device, Offset, ( ULONG )-1 );
	MaskLow = HalPciRead32( Device, Offset );
	HalPciWrite32( Device, Offset, AddressLow );

	if ( AddressLow & PCI_BAR_64 &&
		!( AddressLow & PCI_BAR_IO ) ) {

		Offset += sizeof( ULONG );

		AddressHigh = HalPciRead32( Device, Offset );
		HalPciWrite32( Device, Offset, ( ULONG )-1 );
		MaskHigh = HalPciRead32( Device, Offset );
		HalPciWrite32( Device, Offset, AddressHigh );
	}

	BaseAddressRegister->Base = ( ( ( ULONG64 )AddressHigh << 32 ) | AddressLow ) & ~( ( AddressLow & PCI_BAR_IO ) ? 0x3i64 : 0xfi64 );
	BaseAddressRegister->Size = ( ( ( ULONG64 )MaskHigh << 32 ) | MaskLow ) & ~( ( AddressLow & PCI_BAR_IO ) ? 0x3i64 : 0xfi64 );
	BaseAddressRegister->Size = ~BaseAddressRegister->Size + 1;
	BaseAddressRegister->Flags = AddressLow & ( ( AddressLow & PCI_BAR_IO ) ? 0x3i64 : 0xfi64 );
}

VOID
HalPciEnumerate(

)
{

	for ( USHORT Bus = 0; Bus < PCI_MAX_BUSES; Bus++ ) {

		for ( UCHAR Device = 0; Device < PCI_MAX_DEVICES; Device++ ) {

			for ( UCHAR Function = 0; Function < PCI_MAX_FUNCTIONS; Function++ ) {
				PCI_DEVICE PciDevice = { Bus, Device, Function };
				if ( HalPciRead16( &PciDevice, FIELD_OFFSET( PCI_DEVICE_HEADER, VendorId ) ) == ( USHORT )-1 )
					continue;
				HalPciDeviceList.DeviceCount++;
			}
		}
	}

	HalPciDeviceList.PciDevices = ( PPCI_DEVICE )MmAllocateMemory( HalPciDeviceList.DeviceCount * sizeof( PCI_DEVICE ), PAGE_READ | PAGE_WRITE );

	HalPciDeviceList.DeviceCount = 0;
	for ( USHORT Bus = 0; Bus < PCI_MAX_BUSES; Bus++ ) {

		for ( UCHAR Device = 0; Device < PCI_MAX_DEVICES; Device++ ) {

			for ( UCHAR Function = 0; Function < PCI_MAX_FUNCTIONS; Function++ ) {
				PCI_DEVICE PciDevice = { Bus, Device, Function };
				if ( HalPciRead16( &PciDevice, FIELD_OFFSET( PCI_DEVICE_HEADER, VendorId ) ) == ( USHORT )-1 )
					continue;
				HalPciDeviceList.PciDevices[ HalPciDeviceList.DeviceCount ].Bus = Bus;
				HalPciDeviceList.PciDevices[ HalPciDeviceList.DeviceCount ].Device = Device;
				HalPciDeviceList.PciDevices[ HalPciDeviceList.DeviceCount ].Function = Function;

				for ( USHORT i = 0; i < sizeof( PCI_DEVICE_HEADER ); i++ ) {

					*( ( char* )&HalPciDeviceList.PciDevices[ HalPciDeviceList.DeviceCount ].PciHeader + i ) = HalPciRead8( &HalPciDeviceList.PciDevices[ HalPciDeviceList.DeviceCount ], i );
				}

#if 0
				DbgPrint( L"pci: class_code: %x, sub_class: %x, prog_if: %x, device_id: %x, vendor_id: %x, (bus%d, dev%d, slot%d)\n",
					HalPciDeviceList.PciDevices[ HalPciDeviceList.DeviceCount ].PciHeader.ClassCode,
					HalPciDeviceList.PciDevices[ HalPciDeviceList.DeviceCount ].PciHeader.SubClass,
					HalPciDeviceList.PciDevices[ HalPciDeviceList.DeviceCount ].PciHeader.Prog_IF,
					HalPciDeviceList.PciDevices[ HalPciDeviceList.DeviceCount ].PciHeader.DeviceId,
					HalPciDeviceList.PciDevices[ HalPciDeviceList.DeviceCount ].PciHeader.VendorId,
					Bus, Device, Function );
#endif

				HalPciDeviceList.DeviceCount++;
			}
		}
	}
}

VOID
HalPciSetIoEnable(
	__in PPCI_DEVICE Device,
	__in BOOLEAN Enable
)
{

	//
	//	0x7 is mem space enable, io space enable, bus mastering.
	//

	if ( Enable ) {

		Device->PciHeader.Command |= 0x7;
	}
	else {

		Device->PciHeader.Command &= ~0x7;
	}

	HalPciWrite16( Device, FIELD_OFFSET( PCI_DEVICE_HEADER, Command ), Device->PciHeader.Command );
}
