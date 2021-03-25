


#include <carbsup.h>

VOID
RtlInitUnicodeString(
    _Out_ PUNICODE_STRING DestinationString,
    _In_  PCWSTR SourceString OPTIONAL
)
{
    ULONG Length;

    DestinationString->Buffer = ( PWSTR )SourceString;
    if ( ARGUMENT_PRESENT( SourceString ) ) {
        Length = ( ULONG )( wcslen( SourceString ) * sizeof( WCHAR ) );
        DestinationString->Length = ( USHORT )Length;
        DestinationString->MaximumLength = ( USHORT )( Length + sizeof( WCHAR ) );
    }
    else {
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
    }
}

VOID
RtlCopyUnicodeString(
    _Inout_ PUNICODE_STRING DestinationString,
    _In_    PUNICODE_STRING SourceString
)
{
    ULONG Length;

    if ( DestinationString->MaximumLength >= SourceString->MaximumLength ) {
        Length = SourceString->MaximumLength - sizeof( WCHAR );
    }
    else {
        Length = DestinationString->MaximumLength - sizeof( WCHAR );
    }

    RtlCopyMemory( DestinationString->Buffer, SourceString->Buffer, Length );
    *( WCHAR* )( ( CHAR* )DestinationString + Length ) = 0;
}

VOID
RtlCreateUnicodeString(
    _Inout_ PUNICODE_STRING DestinationString,
    _In_    PUNICODE_STRING SourceString
)
{
    DestinationString->Buffer = MmAllocatePoolWithTag( NonPagedPool, SourceString->MaximumLength, ' RTS' );
    DestinationString->Length = SourceString->Length;
    DestinationString->MaximumLength = SourceString->MaximumLength;

    RtlCopyMemory( DestinationString->Buffer, SourceString->Buffer, SourceString->MaximumLength );
}

LONG
RtlCompareUnicodeString(
    _In_ PCUNICODE_STRING String1,
    _In_ PCUNICODE_STRING String2,
    _In_ BOOLEAN CaseInsensitive
)
{
    PWSTR s1, s2, Limit;
    WCHAR c1, c2;

    if ( String1->Length != String2->Length ) {
        return ( LONG )String1->Length - ( LONG )String2->Length;
    }

    s1 = String1->Buffer;
    s2 = String2->Buffer;
    Limit = s1 + ( String1->Length <= String2->Length ? String1->Length : String2->Length );
    if ( CaseInsensitive ) {
        while ( s1 < Limit ) {
            c1 = *s1++;
            c2 = *s2++;
            if ( c1 != c2 && RtlUpperChar( c1 ) != RtlUpperChar( c2 ) ) {
                return ( LONG )c1 - ( LONG )c2;
            }
        }
    }
    else {
        while ( s1 < Limit ) {
            c1 = *s1++;
            c2 = *s2++;
            if ( c1 != c2 ) {
                return ( LONG )c1 - ( LONG )c2;
            }
        }
    }

    return 0;
}

BOOLEAN
RtlPrefixString(
    _In_ PUNICODE_STRING Prefix,
    _In_ PUNICODE_STRING String,
    _In_ BOOLEAN CaseInsensitive
)
{
    PWSTR s1, s2;
    WCHAR c1, c2;
    ULONG n;

    s1 = Prefix->Buffer;
    s2 = String->Buffer;
    n = Prefix->Length;
    if ( String->Length < n ) {
        return FALSE;
    }

    if ( CaseInsensitive ) {
        while ( n ) {
            c1 = *s1++;
            c2 = *s2++;

            if ( c1 != c2 && RtlUpperChar( c1 ) != RtlUpperChar( c2 ) ) {
                return FALSE;
            }

            n--;
        }
    }
    else {
        while ( n ) {
            c1 = *s1++;
            c2 = *s2++;

            if ( c1 != c2 ) {
                return FALSE;
            }

            n--;
        }
    }

    return TRUE;
}

VOID
RtlUpperString(
    _In_ PUNICODE_STRING DestinationString,
    _In_ PUNICODE_STRING SourceString
)
{
    PWSTR s1, s2;
    ULONG n;

    s1 = DestinationString->Buffer;
    s2 = SourceString->Buffer;

    if ( DestinationString->MaximumLength >= SourceString->MaximumLength ) {
        n = SourceString->MaximumLength - sizeof( WCHAR );
    }
    else {
        n = DestinationString->MaximumLength - sizeof( WCHAR );
    }

    while ( n ) {
        *s1 = RtlUpperChar( *s2 );
        s1++, s2++;
        n--;
    }
    *s1 = 0;
}

NTSTATUS
RtlAppendUnicodeStringToString(
    _In_ PUNICODE_STRING DestinationString,
    _In_ PCUNICODE_STRING SourceString
)
{
    ULONG n;

    n = SourceString->Length;

    if ( n == 0 ) {
        return STATUS_SUCCESS;
    }

    if ( n + DestinationString->Length > DestinationString->MaximumLength ) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    RtlMoveMemory( ( CHAR* )DestinationString->Buffer + DestinationString->Length, SourceString->Buffer, n );
    DestinationString->Length += ( USHORT )n;

    return STATUS_SUCCESS;
}
