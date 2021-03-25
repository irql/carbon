


#pragma once

typedef union _KAPIC_REDIRECT {

    struct {
        ULONG64 InterruptVector : 8;
        ULONG64 DeliveryMode : 3;
        ULONG64 DestinationMode : 1;
        ULONG64 DeliveryStatus : 1;
        ULONG64 PinPolarity : 1;
        ULONG64 RemoteIrr : 1;
        ULONG64 TriggerMode : 1;
        ULONG64 Mask : 1;
        ULONG64 Reserved : 39;
        ULONG64 Destination : 8;
    };

    struct {
        ULONG32 Lower;
        ULONG32 Upper;
    };

} KAPIC_REDIRECT, *PKAPIC_REDIRECT;

#define IO_APIC_ID                      0x00
#define IO_APIC_VER                     0x01
#define IO_APIC_ARB                     0x02
#define IO_APIC_REDIRECTION_TABLE(n)    (0x10 + 2 * n)

typedef enum _DELIVERY_MODE {
    DeliveryModeEdge,
    DeliveryModeLevel
} DELIVERY_MODE;

typedef enum _DESTINATION_MODE {
    DestinationModePhysical,
    DestinationModeLogical
} DESTINATION_MODE;

//
// TODO: these two functions are temp public, they should
// be adjusted eventually
//

NTSYSAPI
VOID
HalApicRedirectIrq(
    _In_ ULONG Irq,
    _In_ PKAPIC_REDIRECT Entry
);

#if 0
NTSYSAPI
BOOLEAN
HalIdtInstallHandler(
    _In_ ULONG            Vector,
    _In_ KSERVICE_ROUTINE Service
);
#endif
