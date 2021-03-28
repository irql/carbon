


#include <carbsup.h>
#include "iop.h"
#include "../hal/halp.h"
#include "../mm/mi.h"
#include "../ke/ki.h"

NTSTATUS
IopInvalidRequest(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP           Request
)
{
    DeviceObject;

    Request->IoStatus.Status = STATUS_INVALID_REQUEST;
    Request->IoStatus.Information = 0;
    IoCompleteRequest( Request );

    return STATUS_SUCCESS;
}

PDRIVER_OBJECT
IopCreateDriver(
    _In_ PUNICODE_STRING DriverName
)
{
    PDRIVER_OBJECT DriverObject;

    DriverObject = MmAllocatePoolWithTag( NonPagedPoolZeroed, sizeof( DRIVER_OBJECT ), IO_TAG );

    RtlCopyMemory( &DriverObject->DriverName, DriverName, sizeof( UNICODE_STRING ) );
    __stosq( ( ULONG64* )&DriverObject->MajorFunction, ( ULONG64 )IopInvalidRequest, IRP_MJ_MAX );

    return DriverObject;
}

VOID
IopDriverModule(
    _In_ PVOID          BaseAddress,
    _In_ PDRIVER_OBJECT DriverObject
)
{
    PIMAGE_DOS_HEADER DosHeader;
    PIMAGE_NT_HEADERS NtHeaders;

    DosHeader = BaseAddress;
    NtHeaders = ( PIMAGE_NT_HEADERS )( ( PUCHAR )BaseAddress + DosHeader->e_lfanew );

    DriverObject->DriverLoad = ( PKDRIVER_LOAD )( ( PUCHAR )BaseAddress + NtHeaders->OptionalHeader.AddressOfEntryPoint );
    DriverObject->DriverUnload = NULL;
    DriverObject->DriverVad = MiFindVadByAddress( PsInitialSystemProcess, ( ULONG64 )BaseAddress );
}

NTSTATUS
IoLoadDriver(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING DriverPath
)
{
    NTSTATUS ntStatus;
    HANDLE FileHandle;
    HANDLE SectionHandle;
    IO_STATUS_BLOCK Iosb;
    OBJECT_ATTRIBUTES FileAttributes = { RTL_CONSTANT_STRING( L"\\??\\BootDevice" ), { 0 }, 0 };
    OBJECT_ATTRIBUTES SectionAttributes = { 0 };
    PVOID ImageBaseAddress;
    HANDLE ProcessHandle;

    ntStatus = ObOpenObjectFromPointer( &ProcessHandle,
                                        PsInitialSystemProcess,
                                        PROCESS_ALL_ACCESS,
                                        OBJ_KERNEL_HANDLE,
                                        KernelMode );
    if ( !NT_SUCCESS( ntStatus ) ) {

        //
        //  uh oh..
        //

        return ntStatus;
    }


    RtlCopyMemory( &FileAttributes.RootDirectory, DriverPath, sizeof( UNICODE_STRING ) );

    ImageBaseAddress = NULL;
    ntStatus = ZwCreateFile( &FileHandle,
                             &Iosb,
                             GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | SYNCHRONIZE,
                             &FileAttributes,
                             FILE_OPEN_IF,
                             FILE_SHARE_READ,
                             0u );
    if ( !NT_SUCCESS( ntStatus ) ||
         !NT_SUCCESS( Iosb.Status ) ) {

        ZwClose( ProcessHandle );
        return ntStatus;
    }

    SectionAttributes.ObjectName.Buffer = MmAllocatePoolWithTag( NonPagedPool, 512, IO_TAG );
    SectionAttributes.ObjectName.MaximumLength = 512;
    lstrcpyW( SectionAttributes.ObjectName.Buffer, L"\\Driver\\" );
    lstrcatW( SectionAttributes.ObjectName.Buffer,
              DriverPath->Buffer + FsRtlFileNameIndex( DriverPath ) );
    SectionAttributes.ObjectName.Length = ( USHORT )lstrlenW( SectionAttributes.ObjectName.Buffer );

    ntStatus = ZwCreateSection( &SectionHandle,
                                SECTION_ALL_ACCESS,
                                &SectionAttributes,
                                SEC_EXECUTE | SEC_WRITE | SEC_IMAGE,
                                FileHandle );
    if ( !NT_SUCCESS( ntStatus ) ) {

        MmFreePoolWithTag( SectionAttributes.ObjectName.Buffer, IO_TAG );
        ZwClose( FileHandle );
        ZwClose( ProcessHandle );
        return ntStatus;
    }

    ntStatus = ZwMapViewOfSection( SectionHandle,
                                   ProcessHandle,
                                   &ImageBaseAddress,
                                   0u,
                                   0u,
                                   PAGE_READ | PAGE_WRITE | PAGE_EXECUTE );
    if ( !NT_SUCCESS( ntStatus ) ) {

        ZwClose( FileHandle );
        ZwClose( ProcessHandle );
        ZwClose( SectionHandle );
        MmFreePoolWithTag( SectionAttributes.ObjectName.Buffer, IO_TAG );
        return ntStatus;
    }

    IopDriverModule( ImageBaseAddress, DriverObject );

    ntStatus = DriverObject->DriverLoad( DriverObject );
    if ( !NT_SUCCESS( ntStatus ) ) {

        ZwClose( FileHandle );
        ZwClose( ProcessHandle );
        ZwClose( SectionHandle );
        MmFreePoolWithTag( SectionAttributes.ObjectName.Buffer, IO_TAG );
        return ntStatus;
    }

    RtlDebugPrint( L"load=%s addr=%ull\n",
                   DriverObject->DriverName.Buffer,
                   ( ( PMM_VAD )DriverObject->DriverVad )->Start );

    // :halfmemeright: 
    // NOTE: if the module is not a /dll then it will be null
    ObReferenceObject( ( ( PMM_VAD )DriverObject->DriverVad )->FileObject->SectionObject );

    ZwClose( FileHandle );
    ZwClose( ProcessHandle );
    ZwClose( SectionHandle );

    return STATUS_SUCCESS;
}
