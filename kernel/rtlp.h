


#pragma once

#include "pesup.h"

PVAD
RtlpFindTargetModule(
	__in PKTHREAD Thread,
	__in PCONTEXT TargetContext
);

NTSTATUS
RtlpUnwindPrologue(
	__in PKTHREAD Thread,
	__in PCONTEXT TargetContext,
	__in PVOID    TargetVadBase,
	__in PIMAGE_RUNTIME_FUNCTION_ENTRY FunctionEntry
);

