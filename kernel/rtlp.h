


#pragma once

#include "pesup.h"

#define RtlTrapFrameToContext( TrapFrame, Context )\
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

#define RtlContextToTrapFrame( TrapFrame, Context )\
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

//
//  definition for __C_specific_handler's
//  called when an exception occurs during 
//  a __try (dispatcher concept)
//

typedef VOID( *PEXCEPTION_HANDLER )( 
    __in PEXCEPTION_RECORD ExceptionRecord
    );

PVAD
RtlpFindTargetModule(
	__in PKTHREAD Thread,
	__in PCONTEXT TargetContext
);

NTSTATUS
RtlpUnwindPrologue(
	__in PKTHREAD                       Thread,
	__in PCONTEXT                       TargetContext,
	__in PVOID                          TargetVadBase,
	__in PIMAGE_RUNTIME_FUNCTION_ENTRY  FunctionEntry
);

NTSTATUS
RtlpFindTargetExceptionHandler(
    __in    PEXCEPTION_RECORD   ExceptionRecord,
	__inout PVAD*               CatchVad,
	__inout PEXCEPTION_HANDLER* CatchHandler,
	__inout PSCOPE_TABLE*       CatchScope
);

VOID
RtlpEvaluateException(
    __in PEXCEPTION_RECORD  ExceptionRecord,
    __in ULONG64            ExceptionInterrupt
);

#define INT_DIV_BY_ZERO             0x00
#define INT_DEBUG                   0x01
#define INT_NMI                     0x02
#define INT_BREAKPOINT              0x03
#define INT_OVERFLOW                0x04
#define INT_BOUND_RANGE             0x05
#define INT_INVALID_OP              0x06
#define INT_DEVICE_NOT_AVAILABLE    0x07
#define INT_DOUBLE_FAULT            0x08
#define INT_INVALID_TSS             0x0A
#define INT_SEGMENT_NOT_PRESENT     0x0B
#define INT_STACK_SEGMENT           0x0C
#define INT_GENERAL_PROTECTION      0x0D
#define INT_PAGE_FAULT              0x0E
#define INT_FPU_EXCEPTION           0x10
#define INT_ALIGNMENT_CHECK         0x11
#define INT_MACHINE_CHECK           0x12
#define INT_SIMD_EXCEPTION          0x13
#define INT_VIRTUAL_EXCEPTION       0x14
#define INT_SECURITY_EXCEPTION      0x1E

#define E_DE INT_DIV_BY_ZERO
#define E_DB INT_DEBUG
#define E_BP INT_BREAKPOINT
#define E_OF INT_OVERFLOW
#define E_BR INT_BOUND_RANGE
#define E_UD INT_INVALID_OP
#define E_NM INT_DEVICE_NOT_AVAILABLE
#define E_DF INT_DOUBLE_FAULT
#define E_TS INT_INVALID_TSS
#define E_NP INT_SEGMENT_NOT_PRESENT
#define E_SS INT_STACK_SEGMENT
#define E_GP INT_GENERAL_PROTECTION
#define E_PF INT_PAGE_FAULT
#define E_MF INT_FPU_EXCEPTION
#define E_AC INT_ALIGNMENT_CHECK
#define E_XM INT_SIMD_EXCEPTION
#define E_XF E_XM
#define E_VE INT_VIRTUAL_EXCEPTION
#define E_SX INT_SECURITY_EXCEPTION