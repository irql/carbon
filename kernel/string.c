/*++

Module ObjectName:

	string.c

Abstract:

	Defines run-time library string routines for the system.

--*/

#include <carbsup.h>

/* Retrieves string length. */
ULONG lstrlenW(
	__in PCWSTR String // String.
) {
	ULONG Length = 0;

	while ( String[ Length++ ] != 0 );
	return Length - 1; // Return the length excluding a null terminator.
}

/* Compares 2 strings. (Case-sensitive) */
LONG lstrcmpW(
	__in PCWSTR String1,	// String 1.
	__in PCWSTR String2		// String 2.
) {
	while ( *String1++ == *String2++ )
		if ( *String1 == 0 ) return 0;		// Return  0 if strings are the same.

	if ( *--String1 > *--String2 ) return 1;	// Return  1 if String1 is bigger.
	else return -1;							// Return -1 if String2 is bigger.
}

/* Compares 2 strings. */
LONG lstrcmpiW(
	__in PCWSTR String1,	// String 1.
	__in PCWSTR String2		// String 2.
) {
	while ( toupper( *String1 ) == toupper( *String2 ) )
		if ( *String1++ == 0 || *String2++ == 0 ) return 0;	// Return  0 if strings are the same.

	if ( toupper( *String1 ) > toupper( *String2 ) ) return 1;	// Return  1 if String1 is bigger.
	else return -1;											// Return -1 if String2 is bigger.
}

/* Compares 2 string by length. (Case-sensitive) */
LONG lstrncmpW(
	__in PCWSTR String1,	// String 1.
	__in PCWSTR String2,	// String 2.
	__in ULONG	Characters	// Characters to compare.
) {
	while ( *String1++ == *String2++ && --Characters )
		if ( *String1 == 0 ) return 0; // Return 0 if characters are the same.

	return Characters; // Return the amount of different characters.
}

/* Compares 2 strings by length. */
LONG lstrncmpiW(
	__in PCWSTR String1,	// String 1.
	__in PCWSTR String2,	// String 2.
	__in ULONG	Characters	// Characters to compare.
) {
	while ( toupper( *String1 ) == toupper( *String2 ) && --Characters )
		if ( *String1++ == 0 || *String2++ == 0 ) return 0; // Return 0 if characters are the same.

	return Characters; // Return the amount of different characters.
}

/* Copies contents from one string to another. */
VOID lstrcpyW(
	__inout PWSTR	DestinationString,	// Destination string.
	__in	PWSTR	SourceString		// Source string.
) {
	while ( *SourceString != 0 )
		*DestinationString++ = *SourceString++;

	*DestinationString = 0;
}

/* Copies a certain amount of characters from one string to another. */
VOID lstrncpyW(
	__inout PWSTR DestinationString,	// Destination string.
	__in	PWSTR SourceString,			// Source string.
	__in	ULONG Characters			// Characters to copy.
) {
	while ( *SourceString != 0 && Characters-- )
		*DestinationString++ = *SourceString++;

	*DestinationString = 0;
}

/* Concatenates 2 strings. */
VOID lstrcatW(
	__in PWSTR	DestinationString,	// Destination string.
	__in PCWSTR SourceString		// Source string.
) {
	while ( *++DestinationString != 0 );
	while ( *SourceString != 0 ) *DestinationString++ = *SourceString++;

	*DestinationString = 0;
}

/* Concatenates amount of characters of the second string with the first one. */
VOID lstrncatW(
	__in PWSTR	DestinationString,	// Destination string.
	__in PCWSTR SourceString,		// Source string.
	__in ULONG	Characters			// Characters to concatenate.
) {
	while ( *++DestinationString != 0 );
	while ( *SourceString != 0 && Characters-- ) *DestinationString++ = *SourceString++;

	*DestinationString = 0;
}

/* Locates the first occurrence of character in the string. (Case-sensitive) */
PWSTR lstrchrW(
	__in PWSTR String,		// String.
	__in WCHAR Character	// Character.
) {
	while ( *String++ != 0 )
		if ( *String == Character ) return String; // Return pointer to the first occurence.

	return NULL; // Return NULL pointer if no matches.
}

/* Locates the first occurrence of character in the string. */
PWSTR lstrchriW(
	__in PWSTR String,		// String.
	__in WCHAR Character	// Character.
) {
	while ( *String++ != 0 )
		if ( toupper( *String ) == toupper( Character ) ) return String; // Return pointer to the first occurence.

	return NULL; // Return NULL pointer if no matches.
}

/* Locates the first occurence of string in the string. (Case-sensitive) */
PWSTR lstrstrW(
	__in PWSTR String,		// String.
	__in PWSTR Substring	// Substring.
) {
	do {
		PWSTR SubstringAddress = Substring;

		while (*String == *SubstringAddress)
			if (String++, *++SubstringAddress == 0) return String - (SubstringAddress - Substring); // Return pointer to the first occurence.
	} while (*String++ != 0);

	return NULL; // Return NULL pointer if no matches.
}

/* Locates the first occurence of string in the string. */
PWSTR lstrstriW(
	__in PWSTR String,		// String.
	__in PWSTR Substring	// Substring.
) {
	do {
		PWSTR SubstringAddress = Substring;

		while (toupper(*String) == toupper(*SubstringAddress)) {
			if (*SubstringAddress++ == 0) return String - SubstringAddress + Substring; // Return pointer to the first occurence.
			String++;
		}
	} while (*String != 0);

	return NULL; // Return NULL pointer if no matches.
}

/* */
PWSTR lstrtokW(
	__in PWSTR String,		// String.
	__in WCHAR *Delimiters	// Delimiters.
) {
	String;
	Delimiters;

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
	UnicodeString->Length = lstrlenW( String );
	UnicodeString->Size = ( UnicodeString->Length + 1 ) * sizeof( WCHAR );
	UnicodeString->Buffer = String;
}

/* Initializes and allocates a unicode string without a 64-byte alignment. */
VOID RtlAllocateAndInitUnicodeString(
	__inout PUNICODE_STRING	AllocatedUnicodeString, // Allocated unicode string.
	__in	PWSTR			SourceString			// Source string.
) {
	//if ( !NT_SUCCESS( RtlUnicodeStringValidate( AllocatedUnicodeString ) ) ) return; // pog 

	AllocatedUnicodeString->Length = lstrlenW( SourceString );
	AllocatedUnicodeString->Size = ( AllocatedUnicodeString->Length + 1 ) * sizeof( WCHAR );//( ( ( ( ( AllocatedUnicodeString->Length + 1 ) * sizeof( WCHAR ) ) + 63 ) / 64 ) * 64 );
	AllocatedUnicodeString->Buffer = ExAllocatePoolWithTag( AllocatedUnicodeString->Size, TAGEX_STRING );

	lstrcpyW( AllocatedUnicodeString->Buffer, SourceString );
}

/* Initializes and allocates a unicode string. */
VOID RtlAllocateAndInitUnicodeStringEx(
	__inout PUNICODE_STRING	*AllocatedUnicodeString,	// Allocated unicode string.
	__in	PWSTR			SourceString				// Source string.
) {
	*AllocatedUnicodeString = ( PUNICODE_STRING )ExAllocatePoolWithTag( sizeof( UNICODE_STRING ), TAGEX_STRING );

	( *AllocatedUnicodeString )->Length = lstrlenW( SourceString );
	( *AllocatedUnicodeString )->Size = ( ( *AllocatedUnicodeString )->Length + 1 ) * sizeof( WCHAR );

	( *AllocatedUnicodeString )->Buffer = ExAllocatePoolWithTag( ( *AllocatedUnicodeString )->Size, TAGEX_STRING );
	lstrcpyW( ( *AllocatedUnicodeString )->Buffer, SourceString );
}

/* Frees an allocated unicode string. */
VOID RtlFreeUnicodeString(
	__in PUNICODE_STRING AllocatedUnicodeString // Allocated unicode string.
) {
	ExFreePoolWithTag( AllocatedUnicodeString, TAGEX_STRING );
}

/* Copies contents from one string to another. */
NTSTATUS RtlUnicodeStringCopy(
	__inout PUNICODE_STRING DestinationUnicodeString,	// Destination unicode string.
	__in	PUNICODE_STRING SourceUnicodeString			// Source unicode string.
) {
	if ( !NT_SUCCESS( RtlUnicodeStringValidate( DestinationUnicodeString ) ) ||
		!NT_SUCCESS( RtlUnicodeStringValidate( SourceUnicodeString ) ) ) return STATUS_INVALID_PARAMETER;

	lstrcpyW( DestinationUnicodeString->Buffer, SourceUnicodeString->Buffer );

	DestinationUnicodeString->Length = lstrlenW( DestinationUnicodeString->Buffer );
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

	lstrncpyW( DestinationUnicodeString->Buffer, SourceUnicodeString->Buffer, Characters );

	DestinationUnicodeString->Length = lstrlenW( DestinationUnicodeString->Buffer );
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

	return lstrcmpW( UnicodeString1->Buffer, UnicodeString2->Buffer ); // Return the result.
}

/* Compares 2 unicode strings by length. */
ULONG RtlUnicodeStringCompareLength(
	__in PUNICODE_STRING	UnicodeString1,	// Unicode string 1.
	__in PUNICODE_STRING	UnicodeString2,	// Unicode string 2.
	__in ULONG				Characters		// Characters to compare.
) {
	if ( !NT_SUCCESS( RtlUnicodeStringValidate( UnicodeString1 ) ) ||
		!NT_SUCCESS( RtlUnicodeStringValidate( UnicodeString2 ) ) ) return STATUS_INVALID_PARAMETER;

	return lstrncmpW( UnicodeString1->Buffer, UnicodeString2->Buffer, Characters ); // Return the result.
}
