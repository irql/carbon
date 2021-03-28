


#include <carbsup.h>
#include "../hal/halp.h"
#include "ki.h"

KSSDT KiServiceDescriptorTable[ 16 ] = { 0 };

KSYSTEM_SERVICE KiNtServiceTable[ ] = {
    SYSTEM_SERVICE( NtCreateFile, 3 ),
    SYSTEM_SERVICE( NtReadFile, 3 ),
    SYSTEM_SERVICE( NtDeviceIoControlFile, 4 ),
    SYSTEM_SERVICE( NtCreateSection, 1 ),
    SYSTEM_SERVICE( NtMapViewOfSection, 2 ),
    SYSTEM_SERVICE( NtUnmapViewOfSection, 0 ),
    SYSTEM_SERVICE( NtResizeSection, 0 ),
    SYSTEM_SERVICE( NtClose, 0 ),
    SYSTEM_SERVICE( NtCreateThread, 5 ),
    SYSTEM_SERVICE( RtlDebugPrint, 0 ),
    SYSTEM_SERVICE( NtWaitForSingleObject, 0 ),
    SYSTEM_SERVICE( NtAllocateVirtualMemory, 0 ),
    SYSTEM_SERVICE( NtFreeVirtualMemory, 0 ),
    SYSTEM_SERVICE( NtQuerySystemClock, 0 ),
    SYSTEM_SERVICE( NtCreateMutex, 0 ),
    SYSTEM_SERVICE( NtReleaseMutex, 0 ),
    SYSTEM_SERVICE( NtGetTickCount, 0 ),
    SYSTEM_SERVICE( NtQueryInformationFile, 1 ),

};

NTSTATUS
KeInstallServiceDescriptorTable(
    _In_ ULONG32          ServiceTableIndex,
    _In_ ULONG32          ServiceCount,
    _In_ PKSYSTEM_SERVICE ServiceTable
)
{
    if ( KiServiceDescriptorTable[ ServiceTableIndex ].ServiceCount > 0 ) {

        return STATUS_UNSUCCESSFUL;
    }

    KiServiceDescriptorTable[ ServiceTableIndex ].ServiceCount = ServiceCount;
    KiServiceDescriptorTable[ ServiceTableIndex ].ServiceTable = ServiceTable;

    return STATUS_SUCCESS;
}

VOID
KiInitializeServiceCallTable(

)
{
    STATIC BOOLEAN KiNtInitialized = FALSE;

    __writemsr( IA32_MSR_EFER, __readmsr( IA32_MSR_EFER ) | EFER_SCE );

    __writemsr( IA32_MSR_LSTAR, ( unsigned long long )KiFastSystemCall );
    __writemsr( IA32_MSR_STAR, ( ( ( ULONG64 )GDT_USER_CODE64 - 16 ) << 48 ) | ( ( ( ULONG64 )GDT_KERNEL_CODE64 ) << 32 ) );
    __writemsr( IA32_MSR_SFMASK, 0 );

    if ( !KiNtInitialized ) {
        KeInstallServiceDescriptorTable( 0,
                                         sizeof( KiNtServiceTable ) / sizeof( KSYSTEM_SERVICE ),
                                         KiNtServiceTable );

        KiNtInitialized = TRUE;
    }

}
