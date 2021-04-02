


#include <carbsup.h>
#include "psp.h"
#include "../hal/halp.h"
#include "../ke/ki.h"
#include "../ob/obp.h"
#include "../mm/mi.h"

PKPROCESS PsInitialSystemProcess;
HANDLE    PspInitialUserProcessHandle;

VOID
PspLoadKnownDll(
    _Out_ PHANDLE            SectionHandle,
    _In_  POBJECT_ATTRIBUTES FileAttributes,
    _In_  POBJECT_ATTRIBUTES SectionAttributes
)
{
    NTSTATUS ntStatus;
    HANDLE FileHandle;
    IO_STATUS_BLOCK iosb = { 0 };

    FileHandle = 0;

    ntStatus = ZwCreateFile( &FileHandle,
                             &iosb,
                             GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | SYNCHRONIZE,
                             FileAttributes,
                             FILE_OPEN_IF,
                             FILE_SHARE_READ,
                             0 );
    if ( !NT_SUCCESS( ntStatus ) ||
         !NT_SUCCESS( iosb.Status ) ) {

        goto PspProcedureFinished;
    }

    ntStatus = ZwCreateSection( SectionHandle,
                                SECTION_ALL_ACCESS,
                                SectionAttributes,
                                SEC_EXECUTE | SEC_WRITE | SEC_IMAGE,
                                FileHandle );

PspProcedureFinished:;

    if ( FileHandle != 0 ) {

        NtClose( FileHandle );
    }

    //return ntStatus;
}

VOID
PspCreateInitialUserProcess(

)
{
    STATIC OBJECT_ATTRIBUTES InitialProcessFile = {
        RTL_CONSTANT_STRING( L"\\??\\BootDevice" ),
        RTL_CONSTANT_STRING( L"\\SYSTEM\\CARBINIT.EXE" ), OBJ_KERNEL_HANDLE };


    PspCreateUserProcess( &PspInitialUserProcessHandle,
                          PROCESS_ALL_ACCESS,
                          &InitialProcessFile );
#if 0 // pge
    RtlDebugPrint( L"about to read... from cr3=%ull\n", __readcr3( ) );

    RtlDebugPrint( L"reading value on p %ull\n", *( ULONG64* )0x424242000 );
#endif
}

NTSTATUS
PspCreateUserProcess(
    _Out_ PHANDLE            ProcessHandle,
    _In_  ACCESS_MASK        DesiredAccess,
    _In_  POBJECT_ATTRIBUTES ObjectAttributes
)
{
    //
    // This function should only really be called by the kernel initially to 
    // load and initialize the first user mode process, processes which are
    // created later should have most work be done by ntdll.
    //

    ProcessHandle;
    OBJECT_ATTRIBUTES ProcessAttributes = { { 0 }, { 0 }, OBJ_KERNEL_HANDLE };

    NTSTATUS ntStatus;
    PKPROCESS Process;
    HANDLE ThreadHandle;
    HANDLE FileHandle;
    HANDLE SectionHandle;
    OBJECT_ATTRIBUTES SectionAttributes = { { 0 }, { 0 }, OBJ_KERNEL_HANDLE };
    IO_STATUS_BLOCK StatusBlock;
    PVOID BaseAddress;
    PIMAGE_DOS_HEADER DosHeader;
    PIMAGE_NT_HEADERS NtHeaders;
    PKSTART_ROUTINE StartRoutine;
    ULONG64 PreviousAddressSpace;
    OBJECT_ATTRIBUTES ThreadAttributes = { { 0 }, { 0 }, OBJ_KERNEL_HANDLE };
    WCHAR CdBuffer[ 256 ];

    BaseAddress = NULL;

    ntStatus = ObCreateObject( &Process,
                               PsProcessObject,
                               &ProcessAttributes,
                               sizeof( KPROCESS ) );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    ntStatus = ObOpenObjectFromPointer( ProcessHandle,
                                        Process,
                                        DesiredAccess,
                                        ProcessAttributes.Attributes,
                                        KernelMode );
    if ( !NT_SUCCESS( ntStatus ) ) {

        goto done;
    }

    ntStatus = ZwCreateFile( &FileHandle,
                             &StatusBlock,
                             GENERIC_ALL | SYNCHRONIZE,
                             ObjectAttributes,
                             FILE_OPEN_IF,
                             FILE_SHARE_READ,
                             0 );
    if ( !NT_SUCCESS( ntStatus ) ) {

        goto done;
    }

    ntStatus = ZwCreateSection( &SectionHandle,
                                SECTION_ALL_ACCESS,
                                &SectionAttributes,
                                SEC_EXECUTE | SEC_WRITE | SEC_IMAGE,
                                FileHandle );
    if ( !NT_SUCCESS( ntStatus ) ) {

        goto done;
    }

    Process->DirectoryTableBase = MmCreateAddressSpace( );

    ntStatus = ZwMapViewOfSection( SectionHandle,
                                   *ProcessHandle,
                                   &BaseAddress,
                                   0,
                                   0,
                                   PAGE_READ | PAGE_WRITE | PAGE_EXECUTE );
    if ( !NT_SUCCESS( ntStatus ) ) {

        goto done;
    }

    Process->ProcessId = KeGenerateUniqueId( );
    Process->Peb = NULL;
    ZwAllocateVirtualMemory( *ProcessHandle,
                             &Process->Peb,
                             0x1000,
                             PAGE_READ | PAGE_WRITE );

    PreviousAddressSpace = MiGetAddressSpace( );
    MiSetAddressSpace( Process->DirectoryTableBase );
#if 0 // pge lol
    PVOID p = ( PVOID )0x424242000;
    ZwAllocateVirtualMemory( PspInitialUserProcessHandle,
                             &p, 0x1000, PAGE_READ | PAGE_WRITE );
    MmAddressPageTable( ( ULONG64 )p )[ MiIndexLevel1( p ) ].Global = 1;
    MiReferenceLevel3Entry( MiIndexLevel4( p ), MiIndexLevel3( p ) )[ MiIndexLevel2( p ) ].Global = 1;
    RtlDebugPrint( L"global %ull in %ull\n", p, __readcr3( ) );

    for ( int i = 0; i < 0x2222; i++ ) {
        *( ULONG64* )p = 0x42424242;

    }
#endif

    DosHeader = BaseAddress;
    NtHeaders = ( PIMAGE_NT_HEADERS )( ( PUCHAR )BaseAddress + DosHeader->e_lfanew );
    StartRoutine = ( PKSTART_ROUTINE )( ( ULONG64 )BaseAddress +
                                        NtHeaders->OptionalHeader.AddressOfEntryPoint );

    Process->Peb->Pointer = Process->Peb;
    Process->Peb->ProcessHeap = NULL;
    Process->Peb->CurrentDirectory.MaximumLength = 512;
    Process->Peb->CurrentDirectory.Buffer = ( PWSTR )&Process->Peb->CurrentDirectoryBuffer;
    Process->Peb->CurrentDirectory.Length = 0;
    FsRtlContainingDirectory( &ObjectAttributes->RootDirectory,
                              &Process->Peb->CurrentDirectory );
    lstrcatW( Process->Peb->CurrentDirectory.Buffer,
              L"\\" );
    RtlFormatBuffer( CdBuffer, L"%s%s",
                     ObjectAttributes->ObjectName.Buffer +
                     FsRtlFileNameIndex( &ObjectAttributes->ObjectName ),
                     Process->Peb->CurrentDirectoryBuffer );

    lstrcpyW( Process->Peb->CurrentDirectory.Buffer, CdBuffer );
    Process->Peb->CurrentDirectory.Length = ( USHORT )( RtlStringLength( CdBuffer ) * sizeof( WCHAR ) );

    MiSetAddressSpace( PreviousAddressSpace );

    KeInsertTailList( &PsInitialSystemProcess->ProcessLinks, &Process->ProcessLinks );

    ZwCreateThread( &ThreadHandle,
                    *ProcessHandle,
                    THREAD_ALL_ACCESS,
                    StartRoutine,
                    NULL,
                    0,
                    &ThreadAttributes,
                    0x100000,
                    NULL );

    //ObReferenceObject( FileObject->SectionObject );

    ntStatus = STATUS_SUCCESS;
done:;
    //ZwClose( FileHandle );
    //ZwClose( SectionHandle ); IMPORTANT: fix vad refs/sect/file refs 
    //ObDereferenceObject( Process );



    return ntStatus;
}
