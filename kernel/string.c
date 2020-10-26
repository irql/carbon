/*++

Module ObjectName:

	string.c

Abstract:

	Defines run-time library string routines for the system.

--*/

#include <carbsup.h>

/* Retrieves string length. */
ULONG RtlStringLength(
	__in PCWSTR String // String.
) {
	ULONG Length = 0;

	while ( String[ Length ] )
		Length++;

	return Length; // Return the length excluding a null terminator.
}

/* Copies contents from one string to another. */
VOID RtlStringCopy(
	__inout PWSTR	DestinationString,	// Destination string.
	__in	PWSTR	SourceString		// Source string.
) {
	while ( *SourceString != 0 )
		*DestinationString++ = *SourceString++;

	*DestinationString = 0;
}

/* Copies a certain amount of characters from one string to another. */
VOID RtlStringCopyLength(
	__inout PWSTR DestinationString,	// Destination string.
	__in	PWSTR SourceString,			// Source string.
	__in	ULONG Characters			// Characters to copy.
) {
	while ( Characters-- && *SourceString != 0 )
		*DestinationString++ = *SourceString++;

	*DestinationString = 0;
}

/* Compares 2 strings. */
LONG RtlStringCompare(
	__in PCWSTR String1,	// String 1.
	__in PCWSTR String2		// String 2.
) {
	while ( *String1 != 0 && *String2 != 0 &&
		*String1 == *String2 )
		String1++, String2++;

	return *String1 - *String2; // Return the difference between string lengths.
}

/* Compares 2 string by length. */
LONG RtlStringCompareLength(
	__in PCWSTR String1,	// String 1.
	__in PCWSTR String2,	// String 2.
	__in ULONG	Characters	// Characters to compare.
) {
	while ( --Characters &&
		*String1 == *String2 )
		String1++, String2++;

	return *String1 - *String2; // Return the difference between string lengths.
}

/* Validates a unicode string. */
NTSTATUS RtlUnicodeStringValidate(
	__in PUNICODE_STRING UnicodeString // Unicode string.
) {
	if ( UnicodeString == NULL || UnicodeString->Buffer == NULL )
		return STATUS_UNSUCCESSFUL;

	return STATUS_SUCCESS; // Return success if the string is valid.
}

/* Initializes a unicode string. */
VOID RtlInitUnicodeString(
	__inout PUNICODE_STRING	UnicodeString,	// Unicode string.
	__in	PWSTR			String			// Source string.
) {
	UnicodeString->Length = RtlStringLength( String );
	UnicodeString->Size = ( UnicodeString->Length + 1 ) * sizeof( WCHAR );
	UnicodeString->Buffer = String;
}

/* Initializes and allocates a unicode string without a 64-byte alignment. */
VOID RtlAllocateAndInitUnicodeString(
	__inout PUNICODE_STRING	AllocatedUnicodeString, // Allocated unicode string.
	__in	PWSTR			SourceString			// Source string.
) {
	if ( !NT_SUCCESS( RtlUnicodeStringValidate( AllocatedUnicodeString ) ) ) return; // pog 

	AllocatedUnicodeString->Length = RtlStringLength( SourceString );
	AllocatedUnicodeString->Size = ( ( ( ( ( AllocatedUnicodeString->Length + 1 ) * sizeof( WCHAR ) ) + 63 ) / 64 ) * 64 );
	AllocatedUnicodeString->Buffer = ExAllocatePoolWithTag( AllocatedUnicodeString->Size, _byteswap_ulong( 'Str ' ) );

	RtlStringCopy( AllocatedUnicodeString->Buffer, SourceString );
}

/* Initializes and allocates a unicode string. */
VOID RtlAllocateAndInitUnicodeStringEx(
	__inout PUNICODE_STRING	*AllocatedUnicodeString,	// Allocated unicode string.
	__in	PWSTR			SourceString				// Source string.
) {
	*AllocatedUnicodeString = ( PUNICODE_STRING )ExAllocatePoolWithTag( sizeof( UNICODE_STRING ), _byteswap_ulong( 'Str ' ) );

	( *AllocatedUnicodeString )->Length = RtlStringLength( SourceString );
	( *AllocatedUnicodeString )->Size = ( ( *AllocatedUnicodeString )->Length + 1 ) * sizeof( WCHAR );

	( *AllocatedUnicodeString )->Buffer = ExAllocatePoolWithTag( ( *AllocatedUnicodeString )->Size, _byteswap_ulong( 'Str ' ) );
	RtlStringCopy( ( *AllocatedUnicodeString )->Buffer, SourceString );
}

/* Frees an allocated unicode string. */
VOID RtlFreeUnicodeString(
	__in PUNICODE_STRING AllocatedUnicodeString // Allocated unicode string.
) {
	ExFreePoolWithTag( AllocatedUnicodeString, _byteswap_ulong( 'Str ' ) );
}

/* Copies contents from one string to another. */
NTSTATUS RtlUnicodeStringCopy(
	__inout PUNICODE_STRING DestinationUnicodeString,	// Destination unicode string.
	__in	PUNICODE_STRING SourceUnicodeString			// Source unicode string.
) {
	if ( !NT_SUCCESS( RtlUnicodeStringValidate( DestinationUnicodeString ) ) ||
		!NT_SUCCESS( RtlUnicodeStringValidate( SourceUnicodeString ) ) ) return STATUS_INVALID_PARAMETER;

	RtlStringCopy( DestinationUnicodeString->Buffer, SourceUnicodeString->Buffer );

	DestinationUnicodeString->Length = RtlStringLength( DestinationUnicodeString->Buffer );
	DestinationUnicodeString->Size = ( DestinationUnicodeString->Length + 1 ) * sizeof( WCHAR );

	return STATUS_SUCCESS; // Return success if the parameters are valid.
}

/* Copies contents from one string to another. */
NTSTATUS RtlUnicodeStringCopyLength(
	__inout PUNICODE_STRING DestinationUnicodeString,	// Destination unicode string.
	__in	PUNICODE_STRING SourceUnicodeString,		// Source unicode string.
	__in	ULONG			Characters					// Characters to copy.
) {
	if ( !NT_SUCCESS( RtlUnicodeStringValidate( DestinationUnicodeString ) ) ||
		!NT_SUCCESS( RtlUnicodeStringValidate( SourceUnicodeString ) ) ) return STATUS_INVALID_PARAMETER;

	RtlStringCopyLength( DestinationUnicodeString->Buffer, SourceUnicodeString->Buffer, Characters );

	DestinationUnicodeString->Length = RtlStringLength( DestinationUnicodeString->Buffer );
	DestinationUnicodeString->Size = ( DestinationUnicodeString->Length + 1 ) * sizeof( WCHAR );

	return STATUS_SUCCESS; // Return success if the parameters are valid.
}

/* Compares 2 unicode strings. */
ULONG RtlUnicodeStringCompare(
	__in PUNICODE_STRING UnicodeString1,	// Unicode string 1.
	__in PUNICODE_STRING UnicodeString2		// Unicode string 2.
) {
	if ( !NT_SUCCESS( RtlUnicodeStringValidate( UnicodeString1 ) ) ||
		!NT_SUCCESS( RtlUnicodeStringValidate( UnicodeString2 ) ) ) return STATUS_INVALID_PARAMETER;

	return RtlStringCompare( UnicodeString1->Buffer, UnicodeString2->Buffer ); // Return the result.
}

/* Compares 2 unicode strings by length. */
ULONG RtlUnicodeStringCompareLength(
	__in PUNICODE_STRING	UnicodeString1,	// Unicode string 1.
	__in PUNICODE_STRING	UnicodeString2,	// Unicode string 2.
	__in ULONG				Characters		// Characters to compare.
) {
	if ( !NT_SUCCESS( RtlUnicodeStringValidate( UnicodeString1 ) ) ||
		!NT_SUCCESS( RtlUnicodeStringValidate( UnicodeString2 ) ) ) return STATUS_INVALID_PARAMETER;

	return RtlStringCompareLength( UnicodeString1->Buffer, UnicodeString2->Buffer, Characters ); // Return the result.
}
