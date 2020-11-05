


#pragma once

#ifndef NTSYSCALLAPI
#define NTSYSCALLAPI
#endif

NTSYSCALLAPI
NTSTATUS
NtGdiDisplayString(
	__in PUNICODE_STRING String
);

