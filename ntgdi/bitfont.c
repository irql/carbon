


#include "driver.h"
#include "bitfont.h"

NTSTATUS
NtGdiInitializeBitFont(
	__in PKFONT FontObject
)
{

	_memcpy( FontObject->FileBuffer, ( void* )Font8x16, 4096 );

	FontObject->Render = NtGdiBitFontRender;
	FontObject->ExtentPoint = NtGdiBitFontExtentPoint;
	FontObject->EndPoint = NtGdiBitFontEndPoint;

	return STATUS_SUCCESS;
}

VOID
NtGdiBitFontRender(
	__in PKFONT Font,
	__in PWCHAR Text,
	__in ULONG32 x,
	__in ULONG32 y,
	__in ULONG32 Colour,
	__in ULONG32* Buffer,
	__in PRECT Region
)
{
	ULONG32 OriginalX = x;

	for ( ULONG32 k = 0; Text[ k ]; k++ ) {

		CHAR Glyph = ( CHAR )Text[ k ];
		switch ( Glyph ) {
		case '\n':
			y += 16;
		case '\r':
			x = OriginalX;
			break;
		case '\t':
			x += ( 8 * 3 ) + ( 4 - ( x % 4 ) );
			break;
		case ' ':
			x += 8;
			break;
		default:

			for ( UCHAR i = 0; i < 16; i++ ) {
				UCHAR Line = ( ( UCHAR* )Font->FileBuffer )[ Glyph * 16 + i ];

				for ( UCHAR j = 0; j <= 8; j++ ) {
					if ( ( Line >> ( 8 - j - 1 ) ) & 1 ) {

						Buffer[ ( y + i ) * ( Region->right - Region->left ) + x + j ] = Colour;
					}
				}
			}

			x += 8;
			break;
		}

		if ( ( x + 8 ) >= ( Region->right - Region->left ) ) {
			x = OriginalX;
			y += 16;
		}

		if ( y >= ( Region->bottom - Region->top ) ) {

			return;
		}
	}
}

VOID
NtGdiBitFontExtentPoint(
	__in PKFONT Font,
	__in PWCHAR Buffer,
	__in PPOINT Region,
	__in PPOINT ExtentPoint
)
{
	Font;

	ULONG32 x = 0;
	ULONG32 y = 0;

	ExtentPoint->x = 0;
	ExtentPoint->y = 0;

	for ( ULONG32 k = 0; Buffer[ k ]; k++ ) {

		CHAR Glyph = ( CHAR )Buffer[ k ];
		switch ( Glyph ) {
		case '\n':
			y += 16;
		case '\r':
			x = 0;
			break;
		case '\t':
			x += ( 8 * 3 ) + ( 4 - ( x % 4 ) );
			break;
		default:
			x += 8;
			break;
		}

		if ( x > ExtentPoint->x ) {

			ExtentPoint->x = x;
		}

		if ( y > ExtentPoint->y ) {

			ExtentPoint->y = y;
		}

		if ( x >= Region->x ) {

			x = 0;
			y += 16;
		}

		if ( y >= Region->y ) {

			return;
		}
	}
}

VOID
NtGdiBitFontEndPoint(
	__in PKFONT Font,
	__in PWCHAR Buffer,
	__in PPOINT Region,
	__in PPOINT EndPoint
)
{
	Font;

	EndPoint->x = 0;
	EndPoint->y = 0;

	for ( ULONG32 k = 0; Buffer[ k ]; k++ ) {

		CHAR Glyph = ( CHAR )Buffer[ k ];
		switch ( Glyph ) {
		case '\n':
			EndPoint->y += 16;
		case '\r':
			EndPoint->x = 0;
			break;
		case '\t':
			EndPoint->x += ( 8 * 3 ) + ( 4 - ( EndPoint->x % 4 ) );
			break;
		default:
			EndPoint->x += 8;
			break;
		}

		if ( EndPoint->x >= Region->x ) {

			EndPoint->x = 0;
			EndPoint->y += 16;
		}

		if ( EndPoint->y >= Region->y ) {

			return;
		}
	}

}
