


#pragma once

FORCEINLINE
PWCHAR
LdrpNameFromPath(
	__in PWCHAR Path
)
{

	for ( LONG32 i = _wcslen( Path ); i >= 0; i-- ) {

		if ( Path[ i ] == '\\' ) {

			return &Path[ i + 1 ];
		}
	}

	return Path;
}