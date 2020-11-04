/*++

Module ObjectName:

	probe.c

Abstract:

	Memory probing.

--*/


#include <carbsup.h>

VOID
KeProbeForRead(
	__in PVOID Address,
	__in ULONG Length
)
{
	ULONG64_PTR UserAddress = ( ULONG64_PTR )Address;

	if ( UserAddress >= 0x800000000000 ||
		 UserAddress + Length >= 0x800000000000 ) {

		KeRaiseException( STATUS_ACCESS_VIOLATION );
	}

	if ( !MmIsAddressRangeValid( ( PVOID )UserAddress, Length ) ) {

		KeRaiseException( STATUS_ACCESS_VIOLATION );
	}

	return;
}

VOID
KeProbeForWrite(
	__in PVOID Address,
	__in ULONG Length
)
{
	ULONG64_PTR UserAddress = ( ULONG64_PTR )Address;

	if ( UserAddress >= 0x800000000000 ||
		UserAddress + Length >= 0x800000000000 ) {

		KeRaiseException( STATUS_ACCESS_VIOLATION );
	}

	if ( !MmIsAddressRangeValid( ( PVOID )UserAddress, Length ) ) {

		KeRaiseException( STATUS_ACCESS_VIOLATION );
	}

	Length += ( UserAddress & 0xFFF );
	Length = ROUND_TO_PAGES( Length );
	UserAddress &= 0xFFF;

	while ( Length > 0 ) {
		Length -= 0x1000;

		ULONG64 Flags = MmQueryVirtualProtection( ( PVOID )( UserAddress + Length ) );

		if ( ( Flags & PAGE_READ ) == 0 ) {

			KeRaiseException( STATUS_ACCESS_VIOLATION );
		}

		if ( ( Flags & PAGE_WRITE ) == 0 ) {

			KeRaiseException( STATUS_ACCESS_VIOLATION );
		}
	}

	return;
}

VOID
KeProbeStringForRead(
	__in PUNICODE_STRING StringAddress
)
{

	KeProbeForRead( StringAddress, sizeof( UNICODE_STRING ) );

	if ( StringAddress->Length > StringAddress->Size ) {

		KeRaiseException( STATUS_ACCESS_VIOLATION );
	}

	KeProbeForRead( StringAddress->Buffer, StringAddress->Size );

	__try {

		//
		//	this means we've read the entire buffer.
		//

		StringAddress->Length = lstrlenW( StringAddress->Buffer );

		if ( StringAddress->Length > StringAddress->Size ) {

			KeRaiseException( STATUS_ACCESS_VIOLATION );
		}

	}
	__except ( EXCEPTION_EXECUTE_HANDLER ) {

		KeRaiseException( STATUS_ACCESS_VIOLATION );
	}

	return;
}

VOID
KeProbeStringForWrite(
	__in PUNICODE_STRING StringAddress
)
{

	KeProbeForWrite( StringAddress, sizeof( UNICODE_STRING ) );

	if ( StringAddress->Length > StringAddress->Size ) {

		KeRaiseException( STATUS_ACCESS_VIOLATION );
	}

	KeProbeForWrite( StringAddress->Buffer, StringAddress->Size );

	__try {

		//
		//	this means we've read the entire buffer.
		//

		StringAddress->Length = lstrlenW( StringAddress->Buffer );

		if ( StringAddress->Length > StringAddress->Size ) {

			KeRaiseException( STATUS_ACCESS_VIOLATION );
		}

	}
	__except ( EXCEPTION_EXECUTE_HANDLER ) {

		KeRaiseException( STATUS_ACCESS_VIOLATION );
	}

	return;
}

