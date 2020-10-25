

#pragma once

#if 0
NTSTATUS
LdrpSupFindModuleBase(
	__in PVOID FileBase,
	__out PVOID* ModuleBase
);
#endif

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


NTSTATUS
LdrpSupGetInfoBlock(
	__in PVOID ModuleBase,
	__in PLDR_INFO_BLOCK InfoBlock
);

NTSTATUS
LdrpSupLoadModule(
	__in PVOID FileBase,
	__in PLDR_INFO_BLOCK InfoBlock
);

NTSTATUS
LdrSupLoadSupervisorModule(
	__in HANDLE FileHandle,
	__in PLDR_INFO_BLOCK InfoBlock
);

