/*++

Module ObjectName:

	string.c

Abstract:

	Defines run time library string routines for the system.

Todo:

	Rename the UNICODE_STRING MaximumLength member to Size.
	Make UNICODE_STRING Length and Size members ULONG

--*/

#include <carbsup.h>

/* Retrieves string length. */
ULONG RtlStringLength(
	__in PCWSTR String // String.
) {
	ULONG Length = 0;

	while (String[Length])
		Length++;

	return Length; // Return the length excluding a null terminator.
}

/* Copies contents from one string to another. */
VOID RtlStringCopy(
	__inout PWSTR	DestinationString,	// Destination string.
	__in	PWSTR	SourceString		// Source string.
) {
	while (*SourceString != 0)
		*DestinationString++ = *SourceString++;

	*DestinationString = 0;
}

/* Copies a certain amount of characters from one string to another. */
VOID RtlStringCopyLength(
	__inout PWSTR DestinationString,	// Destination string.
	__in	PWSTR SourceString,			// Source string.
	__in	ULONG Characters			// Characters to copy.
) {
	while (Characters-- && *SourceString != 0)
		*DestinationString++ = *SourceString++;

	*DestinationString = 0;
}

/* Compares 2 strings. */
LONG RtlStringCompare(
	__in PCWSTR String1,	// String 1.
	__in PCWSTR String2		// String 2.
) {
	while ( *String1 != 0 && *String2 != 0 &&
			*String1++ == *String2++ );
	
	return *String1 - *String2; // Return the difference between string lengths.
}

/* Compares 2 string by length. */
LONG RtlStringCompareLength(
	__in PCWSTR String1,	// String 1.
	__in PCWSTR String2,	// String 2.
	__in ULONG	Characters	// Characters to compare.
) {
	while ( --Characters &&
			*String1++ == *String2++ );

	return *String1 - *String2; // Return the difference between string lengths.
}

/* Validates a unicode string. */
NTSTATUS RtlUnicodeStringValidate(
	__in PUNICODE_STRING UnicodeString // Unicode string.
) {
	if (UnicodeString == NULL || UnicodeString->Buffer == NULL)
		return STATUS_UNSUCCESSFUL;

	return STATUS_SUCCESS; // Return success if the string is valid.
}

/* Initializes a unicode string. */
VOID RtlInitUnicodeString(
	__inout PUNICODE_STRING	UnicodeString,	// Unicode string.
	__in	PWSTR			String			// Source string.
) {
	UnicodeString->Length = RtlStringLength(String);
	UnicodeString->MaximumLength = (UnicodeString->Length + 1) * sizeof(WCHAR);
	UnicodeString->Buffer = String;
}

/* Initializes and allocates a unicode string. */
VOID RtlAllocateAndInitUnicodeString(
	__inout PUNICODE_STRING	AllocatedUnicodeString, // Allocated unicode string.
	__in	PWSTR			SourceString			// Source string.
) {
	if (!NT_SUCCESS(RtlUnicodeStringValidate(AllocatedUnicodeString))) return; // pog 

	AllocatedUnicodeString->Length = RtlStringLength(SourceString);
	AllocatedUnicodeString->MaximumLength = (((((AllocatedUnicodeString->Length + 1) * sizeof(WCHAR)) + 63) / 64) * 64);
	AllocatedUnicodeString->Buffer = ExAllocatePoolWithTag(AllocatedUnicodeString->MaximumLength, _byteswap_ulong('Str '));

	RtlStringCopy(AllocatedUnicodeString->Buffer, SourceString);
}

/* Initializes and allocates a unicode string without a 64-byte alignment. */
VOID RtlAllocateAndInitUnicodeStringEx(
	__inout PUNICODE_STRING	*AllocatedUnicodeString,	// Allocated unicode string.
	__in	PWSTR			SourceString				// Source string.
) {
	*AllocatedUnicodeString = (PUNICODE_STRING)ExAllocatePoolWithTag(sizeof(UNICODE_STRING), _byteswap_ulong('Str '));

	(*AllocatedUnicodeString)->Length = RtlStringLength(SourceString);
	(*AllocatedUnicodeString)->MaximumLength = ((*AllocatedUnicodeString)->Length + 1) * sizeof(WCHAR);
	(*AllocatedUnicodeString)->Buffer = ExAllocatePoolWithTag((*AllocatedUnicodeString)->MaximumLength, _byteswap_ulong('Str '));

	RtlStringCopy((*AllocatedUnicodeString)->Buffer, SourceString);
}

/* Frees an allocated unicode string. */
VOID RtlFreeUnicodeString(
	__in PUNICODE_STRING AllocatedUnicodeString // Allocated unicode string.
) {
	ExFreePoolWithTag(AllocatedUnicodeString, _byteswap_ulong('Str '));
}

/* Copies contents from one string to another. */
NTSTATUS RtlUnicodeStringCopy(
	__inout PUNICODE_STRING DestinationUnicodeString,	// Destination unicode string.
	__in	PUNICODE_STRING SourceUnicodeString			// Source unicode string.
) {
	if (!NT_SUCCESS(RtlUnicodeStringValidate(DestinationUnicodeString)) ||
		!NT_SUCCESS(RtlUnicodeStringValidate(SourceUnicodeString))) return STATUS_INVALID_PARAMETER;

	RtlStringCopy(DestinationUnicodeString->Buffer, SourceUnicodeString->Buffer);

	DestinationUnicodeString->Length = RtlStringLength(DestinationUnicodeString->Buffer);
	DestinationUnicodeString->MaximumLength = (DestinationUnicodeString->Length + 1) * sizeof(WCHAR);

	return STATUS_SUCCESS; // Return success if the parameters are valid.
}

/* Copies contents from one string to another. */
NTSTATUS RtlUnicodeStringCopyLength(
	__inout PUNICODE_STRING DestinationUnicodeString,	// Destination unicode string.
	__in	PUNICODE_STRING SourceUnicodeString,		// Source unicode string.
	__in	ULONG			Characters					// Characters to copy.
) {
	if (!NT_SUCCESS(RtlUnicodeStringValidate(DestinationUnicodeString)) ||
		!NT_SUCCESS(RtlUnicodeStringValidate(SourceUnicodeString))) return STATUS_INVALID_PARAMETER;

	RtlStringCopyLength(DestinationUnicodeString->Buffer, SourceUnicodeString->Buffer, Characters);

	DestinationUnicodeString->Length = RtlStringLength(DestinationUnicodeString->Buffer);
	DestinationUnicodeString->MaximumLength = (DestinationUnicodeString->Length + 1) * sizeof(WCHAR);

	return STATUS_SUCCESS; // Return success if the parameters are valid.
}

/* Compares 2 unicode strings. */
ULONG RtlUnicodeStringCompare(
	__in PUNICODE_STRING UnicodeString1,	// Unicode string 1.
	__in PUNICODE_STRING UnicodeString2		// Unicode string 2.
) {
	if (!NT_SUCCESS(RtlUnicodeStringValidate(UnicodeString1)) ||
		!NT_SUCCESS(RtlUnicodeStringValidate(UnicodeString2))) return STATUS_INVALID_PARAMETER;

	return RtlStringCompare(UnicodeString1->Buffer, UnicodeString2->Buffer); // Return the result.
}

/* Compares 2 unicode strings by length. */
ULONG RtlUnicodeStringCompareLength(
	__in PUNICODE_STRING	UnicodeString1,	// Unicode string 1.
	__in PUNICODE_STRING	UnicodeString2,	// Unicode string 2.
	__in ULONG				Characters		// Characters to compare.
) {
	if (!NT_SUCCESS(RtlUnicodeStringValidate(UnicodeString1)) ||
		!NT_SUCCESS(RtlUnicodeStringValidate(UnicodeString2))) return STATUS_INVALID_PARAMETER;

	return RtlStringCompareLength(UnicodeString1->Buffer, UnicodeString2->Buffer, Characters); // Return the result.
}



// Crt Balls

int _strcmp( char* str1, char* str2 ) {
	while ( *str1 && *str2 && ( *str1 == *str2 ) )
		str1++, str2++;

	return *str1 - *str2;

	/*while (*str1++ == *str2++)
		;

	return *--str1 - *--str2;*/
}

int _wcscmp( wchar_t* str1, wchar_t* str2 ) {
	while ( *str1 && *str2 && ( *str1 == *str2 ) )
		str1++, str2++;

	return *str1 - *str2;

	/*while (*str1++ == *str2++)
		;

	return *--str1 - *--str2;*/
}

int _strlen( char* str1 ) {
	int i = 0;

	while ( str1[ i ] )
		i++;

	return i;
}

int _wcslen( wchar_t* str1 ) {
	int i = 0;

	while ( str1[ i ] )
		i++;

	return i;
}

char* strrev( char* str1 ) {
	char a;
	for ( int i = 0, j = _strlen( str1 ) - 1; i < j; i++, j-- ) {

		a = str1[ i ];
		str1[ i ] = str1[ j ];
		str1[ j ] = a;

	}

	return str1;
}

wchar_t* wcsrev( wchar_t* str1 ) {
	wchar_t u1;

	for ( int i = 0, j = _wcslen( str1 ) - 1; i < j; i++, j-- ) {

		u1 = str1[ i ];
		str1[ i ] = str1[ j ];
		str1[ j ] = u1;
	}

	return str1;
}

char* ftoa( float n, char* str1, unsigned int precision ) {
	int part0 = ( int )n;

	int i = _strlen( itoa( part0, str1, 10 ) );

	if ( precision != 0 ) {
		int part1 = ( int )( ( n - part0 ) * ( float )_pow( 10.f, precision ) );

		str1[ i++ ] = '.';
		while ( precision-- )
			str1[ i++ ] = '0';
		str1[ i ] = 0;

		while ( part1 )
			str1[ --i ] = ( part1 % 10 ) + '0', part1 /= 10;
	}

	return str1;
}

wchar_t* ftow( float n, wchar_t* str1, unsigned int precision ) {
	int part0 = ( int )n;

	int i = _wcslen( itow( part0, str1, 10 ) ) * sizeof( wchar_t );

	if ( precision != 0 ) {
		int part1 = ( int )( ( n - part0 ) * ( float )_pow( 10.f, precision ) );

		str1[ i++ ] = '.';
		while ( precision-- )
			str1[ i++ ] = '0';
		str1[ i ] = 0;

		while ( part1 )
			str1[ --i ] = ( part1 % 10 ) + '0', part1 /= 10;
	}
	return str1;
}

char* itoa( int n, char* str1, unsigned int base ) {
	int i = 0;
	unsigned char negative = 0;

	if ( n == 0 ) {
		str1[ i++ ] = '0';
		str1[ i ] = 0;

		return str1;
	}

	if ( base == 10 && n < 0 ) {
		negative = 1;
		n = -n;
	}

	while ( n != 0 ) {
		int r = n % base;

		str1[ i++ ] = ( ( char )r > 9 ) ? ( ( char )r - 10 ) + 'a' : ( char )r + '0';

		n /= base;
	}

	if ( negative )
		str1[ i++ ] = '-';

	str1[ i ] = 0;

	return strrev( str1 );
}


wchar_t* itow( int n, wchar_t* str1, unsigned int base ) {
	int i = 0;
	unsigned char negative = 0;

	if ( n == 0 ) {
		str1[ i++ ] = '0';
		str1[ i ] = 0;

		return str1;
	}

	if ( base == 10 && n < 0 ) {
		negative = 1;
		n = -n;
	}

	while ( n != 0 ) {
		int r = n % base;

		str1[ i++ ] = ( ( wchar_t )r > 9 ) ? ( ( wchar_t )r - 10 ) + L'a' : ( wchar_t )r + L'0';
		n /= base;
	}

	if ( negative )
		str1[ i++ ] = '-';

	str1[ i ] = 0;

	return wcsrev( str1 );
}

unsigned int atoi( char* str1 ) {
	unsigned int r = 0, negative = 0, i = 0, base = 10;

	if ( str1[ i ] == '-' ) {
		negative = 1;
		i++;
	}

	if ( str1[ i ] == '0' && str1[ i + 1 ] == 'x' ) {
		base = 16;
		i += 2;
	}

	for ( ; str1[ i ] != 0 && ( ( str1[ i ] >= '0' && str1[ i ] <= '9' ) || ( str1[ i ] >= 'a'&&str1[ i ] <= 'f' ) ); i++ ) {
		if ( ( str1[ i ] >= '0' && str1[ i ] <= '9' ) )
			r = r * base + str1[ i ] - '0';
		else
			r = r * base + ( ( str1[ i ] - 'a' ) + 10 );
	}

	if ( negative )
		r *= ( unsigned )-1;

	return r;
}

unsigned int wtoi( wchar_t* str1 ) {
	unsigned int r = 0, negative = 0, i = 0, base = 10;

	if ( str1[ i ] == '-' ) {
		negative = 1;
		i++;
	}

	if ( str1[ i ] == '0' && str1[ i + 1 ] == 'x' ) {
		base = 16;
		i += 2;
	}

	for ( ; str1[ i ] != 0 && ( ( str1[ i ] >= '0' && str1[ i ] <= '9' ) || ( str1[ i ] >= 'a'&&str1[ i ] <= 'f' ) ); i++ ) {
		if ( ( str1[ i ] >= '0' && str1[ i ] <= '9' ) )
			r = r * base + str1[ i ] - '0';
		else
			r = r * base + ( ( str1[ i ] - 'a' ) + 10 );
	}

	if ( negative )
		r *= ( unsigned )-1;

	return r;
}

void vsprintfA( char* buffer, char* format, va_list args ) {

	unsigned int buffer_index = 0;

	while ( *format ) {
		if ( *format != '%' ) {
			buffer[ buffer_index ] = *format++;
			buffer_index++;
			continue;
		}
		else {
			format++;
		}

		unsigned char hash_tag = 0;
		if ( *format == '#' ) {
			hash_tag = 1;
			format++;
		}

		int zero_pad = 0;
		if ( *format == '.' ) {
			format++;
			zero_pad = atoi( format );
			while ( *format >= '0' && *format <= '9' )
				format++;
		}

		switch ( *format ) {
		case '%':
			format++;
			break;

		case 'x':
		case 'X':
		case 'd':
		case 'i':
		case 'b':
		{
			unsigned int arg = va_arg( args, unsigned int ), base = 10;
			unsigned char uppercase = 0;

			switch ( *format ) {
			case 'X':
				uppercase = 1;
			case 'x':
				base = 16;
				if ( hash_tag ) {
					buffer[ buffer_index++ ] = '0';
					buffer[ buffer_index++ ] = 'x';
				}
				break;
			case 'i':
			case 'd':
				base = 10;
				break;
			case 'b':
				base = 2;
				if ( hash_tag ) {
					buffer[ buffer_index++ ] = '0';
					buffer[ buffer_index++ ] = 'b';
				}
				break;
			default:
				break;
			}

			char buf[ 9 ];
			itoa( arg, buf, base );

			int pad = zero_pad - _strlen( buf );

			if ( pad > 0 ) {
				for ( unsigned char i = 0; i < pad; i++, buffer_index++ )
					buffer[ buffer_index ] = '0';
			}

			for ( unsigned char i = 0; buf[ i ]; i++, buffer_index++ )
				buffer[ buffer_index ] = uppercase ? UPPER( buf[ i ] ) : buf[ i ];

			format++;
			break;
		}
		case 'p':
		case 'P':
		{
			__int64 arg = va_arg( args, __int64 );
			__int32 low = ( __int32 )( arg );
			__int32 high = ( __int32 )( arg >> 32 );
			unsigned char uppercase = 0;

			if ( *format == 'P' )
				uppercase = 1;

			if ( hash_tag ) {
				buffer[ buffer_index++ ] = '0';
				buffer[ buffer_index++ ] = 'x';
			}

			char buf_low[ 9 ], buf_high[ 9 ];
			itoa( low, buf_low, 16 );
			itoa( high, buf_high, 16 );
			int low_pad = 8 - _strlen( buf_low );
			if ( high == 0 )
				low_pad = 0;

			int high_pad = zero_pad - ( _strlen( buf_low ) + low_pad );
			if ( high != 0 )
				high_pad -= _strlen( buf_high );

			if ( high_pad > 0 ) {
				for ( unsigned char i = 0; i < high_pad; i++, buffer_index++ )
					buffer[ buffer_index ] = '0';
			}

			for ( unsigned char i = 0; buf_high[ i ] && high != 0; i++, buffer_index++ )
				buffer[ buffer_index ] = uppercase ? UPPER( buf_high[ i ] ) : buf_high[ i ];

			if ( low_pad > 0 ) {
				for ( unsigned char i = 0; i < low_pad; i++, buffer_index++ )
					buffer[ buffer_index ] = '0';
			}

			for ( unsigned char i = 0; buf_low[ i ]; i++, buffer_index++ )
				buffer[ buffer_index ] = uppercase ? UPPER( buf_low[ i ] ) : buf_low[ i ];

			format++;
			break;
		}
		case 's':
		{
			char* str1 = va_arg( args, char* );

			if ( str1 == NULL )
				str1 = "(null)";
			else if ( str1[ 0 ] == 0 )
				str1 = "(zero)";

			for ( unsigned int i = 0; str1[ i ]; i++, buffer_index++ )
				buffer[ buffer_index ] = str1[ i ];

			format++;
			break;
		}
		case 'w':
		{
			wchar_t* str1 = va_arg( args, wchar_t* );

			if ( str1 == NULL )
				str1 = L"(null)";
			else if ( str1[ 0 ] == 0 )
				str1 = L"(zero)";

			for ( unsigned int i = 0; str1[ i ]; i++, buffer_index++ )
				buffer[ buffer_index ] = str1[ i ] & 0xff;

			format++;
			break;
		}
		case 'f':
		{
			double arg = va_arg( args, double );
			char str1[ 20 ];

			if ( zero_pad == 0 )
				zero_pad = 6;

			ftoa( ( float )arg, str1, zero_pad );

			for ( unsigned char i = 0; str1[ i ]; i++, buffer_index++ )
				buffer[ buffer_index ] = str1[ i ];

			format++;
			break;
		}
		case 'c':
		{
			char arg = va_arg( args, char );

			if ( arg == 0 ) {

				char* str1 = "(zero)";
				for ( unsigned int i = 0; str1[ i ]; i++, buffer_index++ )
					buffer[ buffer_index ] = str1[ i ];
			}
			else
				buffer[ buffer_index++ ] = arg;

			format++;
			break;
		}
		default:
			break;

		}
	}
	buffer[ buffer_index ] = 0;

	return;
}

void vsprintfW( wchar_t* buffer, wchar_t* format, va_list args ) {

	unsigned int buffer_index = 0;

	while ( *format ) {
		if ( *format != '%' ) {
			buffer[ buffer_index ] = *format++;
			buffer_index++;
			continue;
		}
		else {
			format++;
		}

		unsigned char hash_tag = 0;
		if ( *format == '#' ) {
			hash_tag = 1;
			format++;
		}

		int zero_pad = 0;
		if ( *format == '.' ) {
			format++;
			zero_pad = wtoi( format );
			while ( *format >= '0' && *format <= '9' )
				format++;
		}

		switch ( *format ) {
		case '%':
			format++;
			break;

		case 'x':
		case 'X':
		case 'd':
		case 'i':
		case 'b':
		{
			unsigned int arg = va_arg( args, unsigned int ), base = 10;
			unsigned char uppercase = 0;

			switch ( *format ) {
			case 'X':
				uppercase = 1;
			case 'x':
				base = 16;
				if ( hash_tag ) {
					buffer[ buffer_index++ ] = '0';
					buffer[ buffer_index++ ] = 'x';
				}
				break;
			case 'i':
			case 'd':
				base = 10;
				break;
			case 'b':
				base = 2;
				if ( hash_tag ) {
					buffer[ buffer_index++ ] = '0';
					buffer[ buffer_index++ ] = 'b';
				}
				break;
			default:
				break;
			}

			wchar_t buf[ 9 ];
			itow( arg, buf, base );

			int pad = zero_pad - _wcslen( buf );

			for ( unsigned char i = 0; i < pad; i++, buffer_index++ )
				buffer[ buffer_index ] = '0';

			for ( unsigned char i = 0; buf[ i ]; i++, buffer_index++ )
				buffer[ buffer_index ] = uppercase ? UPPER( buf[ i ] ) : buf[ i ];

			format++;
			break;
		}
		case 'p':
		case 'P':
		{
			__int64 arg = va_arg( args, __int64 );
			__int32 low = ( __int32 )( arg );
			__int32 high = ( __int32 )( arg >> 32 );
			unsigned char uppercase = 0;

			if ( *format == 'P' )
				uppercase = 1;

			if ( hash_tag ) {
				buffer[ buffer_index++ ] = '0';
				buffer[ buffer_index++ ] = 'x';
			}

			wchar_t buf_low[ 9 ], buf_high[ 9 ];
			itow( low, buf_low, 16 );
			itow( high, buf_high, 16 );
			unsigned int low_pad = 8 - _wcslen( buf_low );
			if ( high == 0 )
				low_pad = 0;

			unsigned int high_pad = zero_pad - ( _wcslen( buf_low ) + low_pad );
			if ( high != 0 )
				high_pad -= _wcslen( buf_high );

			for ( unsigned char i = 0; i < high_pad; i++, buffer_index++ )
				buffer[ buffer_index ] = '0';

			for ( unsigned char i = 0; buf_high[ i ] && high != 0; i++, buffer_index++ )
				buffer[ buffer_index ] = uppercase ? UPPER( buf_high[ i ] ) : buf_low[ i ];

			for ( unsigned char i = 0; i < low_pad; i++, buffer_index++ )
				buffer[ buffer_index ] = '0';

			for ( unsigned char i = 0; buf_low[ i ]; i++, buffer_index++ )
				buffer[ buffer_index ] = uppercase ? UPPER( buf_low[ i ] ) : buf_low[ i ];

			format++;
			break;
		}
		case 's':
		{
			wchar_t* str1 = va_arg( args, wchar_t* );

			if ( str1 == NULL )
				str1 = L"(null)";
			else if ( str1[ 0 ] == 0 )
				str1 = L"(zero)";

			for ( unsigned int i = 0; str1[ i ]; i++, buffer_index++ )
				buffer[ buffer_index ] = str1[ i ];

			format++;
			break;
		}
		case 'a':
		{
			char* str1 = va_arg( args, char* );

			if ( str1 == NULL )
				str1 = "(null)";
			else if ( str1[ 0 ] == 0 )
				str1 = "(zero)";

			for ( unsigned int i = 0; str1[ i ]; i++, buffer_index++ )
				buffer[ buffer_index ] = ( wchar_t )str1[ i ];

			format++;
			break;
		}
		case 'f':
		{
			float arg = va_arg( args, float );
			wchar_t str1[ 12 ];

			if ( zero_pad == 0 )
				zero_pad = 4;

			ftow( arg, str1, zero_pad );

			for ( unsigned char i = 0; str1[ i ]; i++, buffer_index++ )
				buffer[ buffer_index ] = str1[ i ];

			break;
		}
		case 'c':
		{
			wchar_t arg = va_arg( args, wchar_t );

			if ( arg == 0 ) {

				wchar_t* str1 = L"(zero)";
				for ( unsigned int i = 0; str1[ i ]; i++, buffer_index++ )
					buffer[ buffer_index ] = str1[ i ];
			}
			else
				buffer[ buffer_index++ ] = arg;

			format++;
			break;
		}
		default:
			break;

		}
	}
	buffer[ buffer_index ] = 0;

	return;
}

void sprintfA( char* buffer, char* format, ... ) {
	va_list args;
	va_start( args, format );

	vsprintfA( buffer, format, args );

	va_end( args );
}

void sprintfW( wchar_t* buffer, wchar_t* format, ... ) {
	va_list args;
	va_start( args, format );

	vsprintfW( buffer, format, args );

	va_end( args );
}

int strncmp( char* str1, char* str2, int n ) {
	while ( n-- ) {
		if ( *str1 != *str2 )
			return *( unsigned char* )str1 - *( unsigned char* )str2;
		str1++, str2++;
	}
	return 0;
}

int wcsncmp( wchar_t* str1, wchar_t* str2, int n ) {
	while ( n-- ) {
		if ( *str1 != *str2 )
			return *( unsigned short* )str1 - *( unsigned short* )str2;
		str1++, str2++;
	}
	return 0;
}

