

#pragma once

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
	__in PUNICODE_STRING FileName,
	__in PLDR_INFO_BLOCK InfoBlock
);

