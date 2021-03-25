


BITS        16

SECTION     .text

GLOBAL HalProcessorStartup
GLOBAL HalProcessorStartupEnd

%DEFINE TrampolineBase      1000h
%DEFINE TrampolineAddr( x ) ( ( x ) - HalProcessorStartup + TrampolineBase )

ALIGN       16
HalProcessorStartup:
    cli
    xor     ax, ax
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax

    jmp     0:TrampolineAddr(.ZeroCs)
    .ZeroCs:

    mov     eax, dword [TrampolineAddr(HalProcessorStartupEnd)]
    mov     cr3, eax

    mov     eax, cr4
    or      eax, 1 << 5 | 3 << 9
    mov     cr4, eax

    ;mov     word [TrampolineAddr(HalProcessorStartupEnd) + 4], 23
    ;mov     dword [TrampolineAddr(HalProcessorStartupEnd) + 6], 500h
    lgdt    [TrampolineAddr(HalProcessorStartupEnd) + 4]

    mov     ecx, 0c0000080h
    rdmsr
    or      eax, 1 << 8
    wrmsr

    mov     eax, cr0
    or      eax, 1 << 31 | 1 << 0
    and     eax, ~60000000h
    mov     cr0, eax

    mov     ax, 16
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax

    jmp     8:TrampolineAddr(.Ia32e)
BITS        64

    .Ia32e:
    mov     rax, cr0
    or      rax, 1 << 1
    and     rax, ~(1 << 2 | 3 << 29)
    mov     cr0, rax

    mov     rsp, qword [TrampolineAddr(HalProcessorStartupEnd) + 10]
    sub     rsp, 28h
    mov     rcx, qword [TrampolineAddr(HalProcessorStartupEnd) + 18]
    jmp     rcx
ALIGN       16
HalProcessorStartupEnd: