


#pragma once

#include "pesup.h"

#define TRAPFRAME_TO_CONTEXT( TrapFrame, Context )\
(Context)->CodeSegment = (TrapFrame)->CodeSegment;\
(Context)->StackSegment = (TrapFrame)->StackSegment;\
(Context)->DataSegment = (TrapFrame)->DataSegment;\
(Context)->EFlags = (TrapFrame)->EFlags;\
(Context)->Rax = (TrapFrame)->Rax;\
(Context)->Rcx = (TrapFrame)->Rcx;\
(Context)->Rdx = (TrapFrame)->Rdx;\
(Context)->Rbx = (TrapFrame)->Rbx;\
(Context)->Rsp = (TrapFrame)->Rsp;\
(Context)->Rbp = (TrapFrame)->Rbp;\
(Context)->Rsi = (TrapFrame)->Rsi;\
(Context)->Rdi = (TrapFrame)->Rdi;\
(Context)->R8 = (TrapFrame)->R8;\
(Context)->R9 = (TrapFrame)->R9;\
(Context)->R10 = (TrapFrame)->R10;\
(Context)->R11 = (TrapFrame)->R11;\
(Context)->R12 = (TrapFrame)->R12;\
(Context)->R13 = (TrapFrame)->R13;\
(Context)->R14 = (TrapFrame)->R14;\
(Context)->R15 = (TrapFrame)->R15;\
(Context)->Rip = (TrapFrame)->Rip

#define CONTEXT_TO_TRAPFRAME( TrapFrame, Context )\
(TrapFrame)->CodeSegment = (Context)->CodeSegment;\
(TrapFrame)->StackSegment = (Context)->StackSegment;\
(TrapFrame)->DataSegment = (Context)->DataSegment;\
(TrapFrame)->EFlags = (Context)->EFlags;\
(TrapFrame)->Rax = (Context)->Rax;\
(TrapFrame)->Rcx = (Context)->Rcx;\
(TrapFrame)->Rdx = (Context)->Rdx;\
(TrapFrame)->Rbx = (Context)->Rbx;\
(TrapFrame)->Rsp = (Context)->Rsp;\
(TrapFrame)->Rbp = (Context)->Rbp;\
(TrapFrame)->Rsi = (Context)->Rsi;\
(TrapFrame)->Rdi = (Context)->Rdi;\
(TrapFrame)->R8 = (Context)->R8;\
(TrapFrame)->R9 = (Context)->R9;\
(TrapFrame)->R10 = (Context)->R10;\
(TrapFrame)->R11 = (Context)->R11;\
(TrapFrame)->R12 = (Context)->R12;\
(TrapFrame)->R13 = (Context)->R13;\
(TrapFrame)->R14 = (Context)->R14;\
(TrapFrame)->R15 = (Context)->R15;\
(TrapFrame)->Rip = (Context)->Rip

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

NTSTATUS
RtlpFindTargetExceptionHandler(
	__in    PKTHREAD      Thread,
	__in    PCONTEXT      ExceptionContext,
	__inout PVAD*         ExceptionVad,
	__inout PVOID*        ExceptionHandler,
	__inout PSCOPE_TABLE* ExceptionScope
);