


#include "com.h"

VOID
KdCmdListModules(
	__in PKD_CMDR_LIST_MODULES ListModules
)
{

	ULONG32 ModuleCount = ( ListModules->Base.KdCmdSize - sizeof( KD_CMDR_LIST_MODULES ) ) / sizeof( KD_MODULE );

	for ( ULONG32 i = 0; i < ModuleCount; i++ ) {

		KdPrint( L"%s %#.16I64x %#.16I64x %#.16I64x \n",
			ListModules->Module[ i ].ModuleName,
			ListModules->Module[ i ].ModuleStart,
			ListModules->Module[ i ].ModuleEnd,
			ListModules->Module[ i ].ModuleEnd - ListModules->Module[ i ].ModuleStart );
	}

}