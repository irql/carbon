


#include "driver.h"
#include "svga.h"
#include "svga3d.h"

NTSTATUS
DriverLoad(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    DriverObject;

    DdDriverInitialize( DriverObject );

    return STATUS_SUCCESS;
}
