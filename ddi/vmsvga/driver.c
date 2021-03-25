


#include "driver.h"
#include "svga.h"
#include "svga3d.h"

NTSTATUS
DriverLoad(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    DriverObject;


    KdPrint( L"** vmsvga **\n" );
    DdDriverInitialize( DriverObject );

    return STATUS_SUCCESS;
}
