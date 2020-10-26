/*++

Module ObjectName:

	ex.h

Abstract:

	Placeholder.

--*/

#pragma once

#define TAGEX_BOOT		'TOOB'
#define TAGEX_CPU		' UPC'
#define TAGEX_DISK		'KISD'
#define TAGEX_IDE		' EDI'
#define TAGEX_AHCI		'ICHA'
#define TAGEX_IRP		' PRI'

#define TAGEX_IDLE		'ELDI'
#define TAGEX_DEBUG		' GBD'
#define TAGEX_IO		'  OI'

#define TAGEX_MPLA		'ALPM'
#define TAGEX_MIOA		'AOIM'
#define TAGEX_MISO		'OSIM'
#define TAGEX_MNMI		'IMNM'

#define TAGEX_VBE		' EBV'
#define TAGEX_TSS		' SST'
#define TAGEX_KPCR		'RCPK'
#define TAGEX_DI		'  ID'
#define TAGEX_OB		'  BO'

#define TAGEX_FAT		' TAF'
#define TAGEX_PARTITION 'TRAP'

#define TAGEX_WNDCLASS	'XECW'
#define TAGEX_MESSAGE	' GSM'
#define TAGEX_STRING	' RTS'
#define TAGEX_FILE		'ELIF'
#define TAGEX_PATH		'HTAP'
#define TAGEX_CMD		' DMC'

NTSYSAPI PVOID ExAllocatePoolWithTag(
	__in ULONG NumberOfBytes,
	__in ULONG PoolTag
);

NTSYSAPI VOID ExFreePoolWithTag(
	__in PVOID Pool,
	__in ULONG PoolTag
);

