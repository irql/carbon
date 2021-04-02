


#include <carbsup.h>
#include "mm/mi.h"
#include "ob/obp.h"
#include "io/iop.h"
#include "hal/halp.h"
#include "ke/ki.h"
#include "ps/psp.h"
#include "rtl/ldr/ldrp.h"
#include "config/cmp.h"

EXTERN VOLATILE ULONG64 KiCurrentInitCpu;
EXTERN VOLATILE ULONG64 KiCpuInitComplete;
EXTERN KSPIN_LOCK KiDispatcherLock;

PWSTR
RtlNameFromFile(
    _In_ PWSTR FileName
)
{
    PWSTR End;

    End = FileName;
    while ( *End++ )
        ;

    while ( End != FileName ) {

        if ( *End == '\\' ) {

            return End + 1;
        }
        End--;
    }
    return FileName;
}

VOID
KiRecursiveDirectoryList(
    _In_ POBJECT_DIRECTORY CurrentDirectory,
    _In_ ULONG64 Level
)
{

    do {

        for ( ULONG64 Tabs = 0; Tabs < Level; Tabs++ ) {

            RtlDebugPrint( L"\t" );
        }

        if ( CurrentDirectory->Object != NULL &&
             ObpGetHeaderFromObject( CurrentDirectory->Object )->Type == IoSymbolicLinkObject ) {
            RtlDebugPrint( L"%s (Link) %s\n", CurrentDirectory->Name.Buffer, ( ( POBJECT_SYMBOLIC_LINK )CurrentDirectory->Object )->LinkTarget.Buffer );
        }
        else {
            if ( CurrentDirectory->Object != NULL ) {

                RtlDebugPrint( L"%s (%s)\n", CurrentDirectory->Name.Buffer,
                               ObpGetHeaderFromObject( CurrentDirectory->Object )->Type->Name.Buffer );
            }
            else {

                RtlDebugPrint( L"%s\n", CurrentDirectory->Name.Buffer );
            }
        }

        if ( CurrentDirectory->Object != NULL &&
             ObpGetHeaderFromObject( CurrentDirectory->Object )->Type == ObDirectoryObject ) {
            for ( ULONG64 Tabs = 0; Tabs < Level + 1; Tabs++ ) {

                RtlDebugPrint( L"----" );
            }
            RtlDebugPrint( L"--->\n" );
            KiRecursiveDirectoryList( CurrentDirectory->Object, Level + 1 );
            for ( ULONG64 Tabs = 0; Tabs < Level + 1; Tabs++ ) {

                RtlDebugPrint( L"----" );
            }
            RtlDebugPrint( L"---<\n" );
        }

        CurrentDirectory = CurrentDirectory->DirectoryLink;
    } while ( CurrentDirectory != NULL );
}

VOID
KiInitThread(
    _In_ PLDR_BOOT_INFO BootInfo
)
{

    RtlDebugConsoleInit(
        BootInfo->DisplayWidth,
        BootInfo->DisplayHeight,
        BootInfo->BitsPerPixel,
        BootInfo->Framebuffer );

    if ( __writecr8( 1 ), __readcr8( ) != 1 ) {

        //
        // haxm doesnt virtualize the fucking cr8
        //

        RtlDebugPrint( L"uhm.. sir??\n" );
        KeBugCheck( STATUS_UNSUPPORTED_PROCESSOR );
    }
    __writecr8( 0 );

    RtlDebugPrint( L"Bru'al\n" );

    KeInitializeKernelClock( );
    IoInitializeIoManager( );
    MmPhase1InitializeMemoryManager( );

    PRD_FILE_LIST Rd = ( PRD_FILE_LIST )( ULONG64 )( BootInfo->FileList & 0xFFFF );

    PMM_VAD Vad;
    PIMAGE_DOS_HEADER DosHeader;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_SECTION_HEADER LastSection;
    PIMAGE_SECTION_HEADER SectionHeaders;
    PIMAGE_IMPORT_DESCRIPTOR Import;
    OBJECT_ATTRIBUTES Attributes = { { 0 }, { 0 }, OBJ_KERNEL_HANDLE };
    UNICODE_STRING RootName = RTL_CONSTANT_STRING( L"\\SYSTEM\\CARBKRNL.SYS" );
    UNICODE_STRING FileName;
    ULONG64 CurrentEntry;
    ULONG64 CurrentChar;
    ULONG64 CurrentSection;
    PMM_VAD CurrentVad;
    WCHAR ConversionBuffer[ 256 ] = { 0 };
    PCHAR ImportNameBuffer;

    DosHeader = ( PIMAGE_DOS_HEADER )( 0xFFFFFFFFFFE00000 );
    NtHeaders = ( PIMAGE_NT_HEADERS )( 0xFFFFFFFFFFE00000 + DosHeader->e_lfanew );
    LastSection = &IMAGE_FIRST_SECTION( NtHeaders )[ NtHeaders->FileHeader.NumberOfSections - 1 ];

    Vad = MiAllocateVad( );
    Vad->Start = 0xFFFFFFFFFFE00000;
    LdrpGetLoaderLimits( ( PVOID )Vad->Start, &Vad->End );
    Vad->End += Vad->Start;
    Vad->Charge = Vad->End - Vad->Start;

    ObCreateObject( &Vad->FileObject, IoFileObject, &Attributes, sizeof( IO_FILE_OBJECT ) );

    Vad->FileObject->FileName = RootName;
    MiInsertVad( PsInitialSystemProcess, Vad );

    //
    // The order at which files are placed within the FAT32 directory matters.
    // this should eventually be changed for some form of order.
    //
    // The order should be:
    //  pci.sys
    //  pciide.sys
    //  partmgr.sys
    //  fat.sys
    //
    // This is a pretty poor system and should eventually be adapted to work wtih
    // other drivers, partmgr expects disks to be already created and searches for
    // them using a few object directory hacks.
    //
    // Implement v8086.
    //

    ULONG64 CurrentLoad = 0;
    STATIC PCHAR KiLoadOrder[ ] = {
        "KDCOM   SYS",
        "PCI     SYS",
        "PCIIDE  SYS",
        "PARTMGR SYS",
        "FAT     SYS",
    };

    for ( CurrentLoad = 0; CurrentLoad < sizeof( KiLoadOrder ) / sizeof( PCHAR ); CurrentLoad++ ) {

        for ( CurrentEntry = 0; CurrentEntry < Rd->EntryCount; CurrentEntry++ ) {

            if ( RtlCompareAnsiString( KiLoadOrder[ CurrentLoad ],
                                       Rd->Entries[ CurrentEntry ].DirectoryFile,
                                       TRUE ) != 0 ) {

                continue;
            }

            Rd->Entries[ CurrentEntry ].FilePointer =
                ( ( Rd->Entries[ CurrentEntry ].FilePointer >> 12 ) & 0xFFFF0 ) |
                ( Rd->Entries[ CurrentEntry ].FilePointer & 0xFFFF );
            Rd->Entries[ CurrentEntry ].Length =
                ( ( Rd->Entries[ CurrentEntry ].Length >> 12 ) & 0xFFFF0 ) |
                ( Rd->Entries[ CurrentEntry ].Length & 0xFFFF );

            FileName.MaximumLength = 256;
            FileName.Buffer = MmAllocatePoolWithTag( NonPagedPoolZeroed, 256, KE_TAG );

            lstrcpyW( FileName.Buffer, L"\\SYSTEM\\BOOT\\" );

            for (
                CurrentChar = 0;
                CurrentChar < 11 &&
                Rd->Entries[ CurrentEntry ].DirectoryFile[ CurrentChar ] != 0 &&
                Rd->Entries[ CurrentEntry ].DirectoryFile[ CurrentChar ] != ' ';
                CurrentChar++ ) {

                FileName.Buffer[ lstrlenW( FileName.Buffer ) ] =
                    ( WCHAR )Rd->Entries[ CurrentEntry ].DirectoryFile[ CurrentChar ];
            }

            FileName.Buffer[ lstrlenW( FileName.Buffer ) ] = '.';

            for ( CurrentChar = 0; CurrentChar < 3; CurrentChar++ ) {
                FileName.Buffer[ lstrlenW( FileName.Buffer ) ] = ( WCHAR )Rd->Entries[ CurrentEntry ].DirectoryFile[ 8 + CurrentChar ];
            }

            FileName.Length = ( USHORT )( lstrlenW( FileName.Buffer ) * sizeof( WCHAR ) );

            DosHeader = ( PIMAGE_DOS_HEADER )( ( ULONG64 )Rd->Entries[ CurrentEntry ].FilePointer );
            NtHeaders = ( PIMAGE_NT_HEADERS )( ( ULONG64 )Rd->Entries[ CurrentEntry ].FilePointer + DosHeader->e_lfanew );
            SectionHeaders = IMAGE_FIRST_SECTION( NtHeaders );
            LastSection = &SectionHeaders[ NtHeaders->FileHeader.NumberOfSections - 1 ];

            Vad = MiAllocateVad( );
            //Vad->Charge = LastSection->VirtualAddress + ROUND_TO_PAGES( LastSection->Misc.VirtualSize );
            LdrpGetLoaderLimits( DosHeader, &Vad->Charge );
            Vad->Start = ( ULONG64 )MmAllocatePoolWithTag( NonPagedPoolZeroedExecute,
                                                           Vad->Charge,
                                                           KE_TAG );
            Vad->End = Vad->Start + Vad->Charge;

            //
            // Temp FO's for boot drivers, fix eventually.
            //

            ObCreateObject( &Vad->FileObject,
                            IoFileObject,
                            &Attributes,
                            sizeof( IO_FILE_OBJECT ) );

            Vad->FileObject->FileName = FileName;
            MiInsertVad( PsInitialSystemProcess, Vad );

            RtlCopyMemory( ( PVOID )Vad->Start, DosHeader, NtHeaders->OptionalHeader.SizeOfHeaders );

            for ( CurrentSection = 0; CurrentSection < NtHeaders->FileHeader.NumberOfSections; CurrentSection++ ) {

                RtlCopyMemory(
                    ( PVOID )( Vad->Start + SectionHeaders[ CurrentSection ].VirtualAddress ),
                    ( PVOID )( ( ( PUCHAR )DosHeader ) + SectionHeaders[ CurrentSection ].PointerToRawData ),
                    SectionHeaders[ CurrentSection ].SizeOfRawData );
            }

            LdrResolveBaseReloc( DosHeader );

            Import = ( PVOID )( Vad->Start +
                                NtHeaders->OptionalHeader.
                                DataDirectory[ IMAGE_DIRECTORY_ENTRY_IMPORT ].VirtualAddress );

            while ( Import->Characteristics ) {

                CurrentVad = PsInitialSystemProcess->VadRoot;
                ImportNameBuffer = ( PVOID )( Vad->Start + Import->Name );
                for ( CurrentChar = 0; ImportNameBuffer[ CurrentChar ] != 0; CurrentChar++ ) {

                    ConversionBuffer[ CurrentChar ] = ImportNameBuffer[ CurrentChar ];
                    ConversionBuffer[ CurrentChar + 1 ] = 0;
                }

                while ( CurrentVad != NULL ) {

                    if ( RtlCompareString(
                        RtlNameFromFile( CurrentVad->FileObject->FileName.Buffer ),
                        ConversionBuffer, TRUE ) == 0 ) {
                        LdrResolveImportTable(
                            ( PVOID )( Vad->Start ),
                            ( PVOID )( CurrentVad->Start ),
                            Import );
                    }

                    CurrentVad = CurrentVad->Link;
                }

                Import++;
            }

            RtlDebugPrint( L"file=%s addr=%ull\n",
                           Vad->FileObject->FileName.Buffer,
                           Vad->Start );
            /*
            RtlDebugPrint( L"rd map=%s "
                           L"start=%ull "
                           L"end=%ull "
                           L"entry=%ull\n",
                           Vad->FileObject->FileName.Buffer,
                           Vad->Start, Vad->End,
                           Vad->Start + NtHeaders->OptionalHeader.AddressOfEntryPoint );*/
            ( ( PKDRIVER_LOAD )( Vad->Start + NtHeaders->OptionalHeader.AddressOfEntryPoint ) )(
                IopCreateDriver( &Vad->FileObject->FileName ) );
        }

    }

    //CmInitializeConfigManager( );

    // also fix up parts of the kernel to use 
    // RTL_STACK_STRING instead of MmAllocatePoolWithTag.

    //
    // Initialization code is always going to be
    // horrible, we have to manually setup 
    // a section object and vad for the kernel
    // after rd finishes. (fat driver and disk
    // driver are required for proper FileObject)
    //
    // Important: implement something similar to 
    // IoRegisterDriverReinitialization
    //
    // Look into:
    // IopLoadFileSystemDriver
    //

    NTSTATUS ntStatus;
    HANDLE FileHandle;
    IO_STATUS_BLOCK FileStatus;
    STATIC OBJECT_ATTRIBUTES FileAttributes = {
        RTL_CONSTANT_STRING( L"\\??\\BootDevice" ),
        RTL_CONSTANT_STRING( L"\\SYSTEM\\CARBKRNL.SYS" ), OBJ_KERNEL_HANDLE };
    PIO_FILE_OBJECT FileObject;
    PMM_SECTION_OBJECT SectionObject;
    OBJECT_ATTRIBUTES SectionAttributes = { { 0 }, { 0 }, OBJ_KERNEL_HANDLE };

    ntStatus = ZwCreateFile( &FileHandle,
                             &FileStatus,
                             GENERIC_ALL | SYNCHRONIZE,
                             &FileAttributes,
                             FILE_OPEN_IF,
                             FILE_SHARE_READ,
                             0u );
    if ( !NT_SUCCESS( ntStatus ) ) {

        KeBugCheck( STATUS_KERNEL_INITIALIZATION_FAILURE );
    }

    ntStatus = ObReferenceObjectByHandle( &FileObject,
                                          FileHandle,
                                          0,
                                          KernelMode,
                                          IoFileObject );
    if ( !NT_SUCCESS( ntStatus ) ) {

        KeBugCheck( STATUS_KERNEL_INITIALIZATION_FAILURE );
    }

    CurrentVad = PsInitialSystemProcess->VadRoot;
    CurrentVad->FileObject = FileObject;
    CurrentVad->Start = 0xFFFFFFFFFFE00000;
    LdrpGetLoaderLimits( ( PVOID )CurrentVad->Start, &CurrentVad->End );
    CurrentVad->End += CurrentVad->Start;
    CurrentVad->Charge = CurrentVad->End - CurrentVad->Start;

    ntStatus = MmCreateSectionSpecifyAddress( &SectionObject,
                                              &SectionAttributes,
                                              SEC_EXECUTE | SEC_IMAGE | SEC_WRITE,
                                              0x200000,
                                              CurrentVad->Charge );
    if ( !NT_SUCCESS( ntStatus ) ) {

        KeBugCheck( STATUS_KERNEL_INITIALIZATION_FAILURE );
    }
    FileObject->SectionObject = SectionObject;

    MM_WSLE Entry;

    Entry.Upper = 0;
    Entry.Lower = 0;
    Entry.Usage = MmMappedViewOfSection;
    Entry.TypeMappedViewOfSection.SectionObject = ( ULONG64 )SectionObject;
    Entry.TypeMappedViewOfSection.LengthLower = CurrentVad->Charge >> 12;
    Entry.TypeMappedViewOfSection.LengthUpper = CurrentVad->Charge >> 20;
    Entry.TypeMappedViewOfSection.Address = 0xFFFFFFFFFFE00000 >> 12;

    KIRQL PreviousIrql;

    KeAcquireSpinLock( &PsInitialSystemProcess->WorkingSetLock, &PreviousIrql );
    MmInsertWorkingSet( &Entry );
    KeReleaseSpinLock( &PsInitialSystemProcess->WorkingSetLock, PreviousIrql );

    OBJECT_ATTRIBUTES dxgi = {
        RTL_CONSTANT_STRING( L"\\??\\BootDevice" ),
        RTL_CONSTANT_STRING( L"\\SYSTEM\\DXGI.SYS" ), OBJ_KERNEL_HANDLE };
    ntStatus = IoLoadDriver( IopCreateDriver( &dxgi.RootDirectory ),
                             &dxgi.RootDirectory );
    if ( !NT_SUCCESS( ntStatus ) ) {

        KeBugCheck( STATUS_KERNEL_INITIALIZATION_FAILURE );
    }
#if 0
    OBJECT_ATTRIBUTES vmsvga = {
        RTL_CONSTANT_STRING( L"\\??\\BootDevice" ),
        RTL_CONSTANT_STRING( L"\\SYSTEM\\VMSVGA.SYS" ), OBJ_KERNEL_HANDLE };
    ntStatus = IoLoadDriver( IopCreateDriver( &vmsvga.RootDirectory ),
                             &vmsvga.RootDirectory );
    if ( !NT_SUCCESS( ntStatus ) ) {

        KeBugCheck( STATUS_KERNEL_INITIALIZATION_FAILURE );
    }
#endif
    OBJECT_ATTRIBUTES vesa = {
        RTL_CONSTANT_STRING( L"\\??\\BootDevice" ),
        RTL_CONSTANT_STRING( L"\\SYSTEM\\VESA.SYS" ), OBJ_KERNEL_HANDLE };
    ntStatus = IoLoadDriver( IopCreateDriver( &vesa.RootDirectory ),
                             &vesa.RootDirectory );
    if ( !NT_SUCCESS( ntStatus ) ) {

        KeBugCheck( STATUS_KERNEL_INITIALIZATION_FAILURE );
    }

    OBJECT_ATTRIBUTES ntuser = {
        RTL_CONSTANT_STRING( L"\\??\\BootDevice" ),
        RTL_CONSTANT_STRING( L"\\SYSTEM\\NTUSER.SYS" ), OBJ_KERNEL_HANDLE };
    ntStatus = IoLoadDriver( IopCreateDriver( &ntuser.RootDirectory ),
                             &ntuser.RootDirectory );
    if ( !NT_SUCCESS( ntStatus ) ) {

        KeBugCheck( STATUS_KERNEL_INITIALIZATION_FAILURE );
    }
#if 1
    OBJECT_ATTRIBUTES i8042 = {
        RTL_CONSTANT_STRING( L"\\??\\BootDevice" ),
        RTL_CONSTANT_STRING( L"\\SYSTEM\\I8042.SYS" ), OBJ_KERNEL_HANDLE };
    ntStatus = IoLoadDriver( IopCreateDriver( &i8042.RootDirectory ),
                             &i8042.RootDirectory );
    if ( !NT_SUCCESS( ntStatus ) ) {

        KeBugCheck( STATUS_KERNEL_INITIALIZATION_FAILURE );
    }
#endif
    HANDLE ntdll_Handle;
    OBJECT_ATTRIBUTES ntdll_File = {
        RTL_CONSTANT_STRING( L"\\??\\BootDevice" ),
        RTL_CONSTANT_STRING( L"\\SYSTEM\\NTDLL.DLL" ), OBJ_KERNEL_HANDLE };
    OBJECT_ATTRIBUTES ntdll_Section = {
        RTL_CONSTANT_STRING( L"\\KnownDlls\\NTDLL.DLL" ), { 0 }, OBJ_KERNEL_HANDLE };
    PspLoadKnownDll( &ntdll_Handle, &ntdll_File, &ntdll_Section );

    HANDLE d3d_Handle;
    OBJECT_ATTRIBUTES d3d_File = {
        RTL_CONSTANT_STRING( L"\\??\\BootDevice" ),
        RTL_CONSTANT_STRING( L"\\SYSTEM\\D3D.DLL" ), OBJ_KERNEL_HANDLE };
    OBJECT_ATTRIBUTES d3d_Section = {
        RTL_CONSTANT_STRING( L"\\KnownDlls\\D3D.DLL" ), { 0 }, OBJ_KERNEL_HANDLE };
    PspLoadKnownDll( &d3d_Handle, &d3d_File, &d3d_Section );

    //
    // because user.dll relies on this, i want it to be a knowndll
    // it is the font driver for the system pretty much
    //

    HANDLE freetype_Handle;
    OBJECT_ATTRIBUTES freetype_File = {
        RTL_CONSTANT_STRING( L"\\??\\BootDevice" ),
        RTL_CONSTANT_STRING( L"\\SYSTEM\\FREETYPE.DLL" ), OBJ_KERNEL_HANDLE };
    OBJECT_ATTRIBUTES freetype_Section = {
        RTL_CONSTANT_STRING( L"\\KnownDlls\\FREETYPE.DLL" ), { 0 }, OBJ_KERNEL_HANDLE };
    PspLoadKnownDll( &freetype_Handle, &freetype_File, &freetype_Section );

    HANDLE user_Handle;
    OBJECT_ATTRIBUTES user_File = {
        RTL_CONSTANT_STRING( L"\\??\\BootDevice" ),
        RTL_CONSTANT_STRING( L"\\SYSTEM\\USER.DLL" ), OBJ_KERNEL_HANDLE };
    OBJECT_ATTRIBUTES user_Section = {
        RTL_CONSTANT_STRING( L"\\KnownDlls\\USER.DLL" ), { 0 }, OBJ_KERNEL_HANDLE };
    PspLoadKnownDll( &user_Handle, &user_File, &user_Section );

    PspCreateInitialUserProcess( );

    HANDLE explorer_Handle;
    OBJECT_ATTRIBUTES explorer_File = {
        RTL_CONSTANT_STRING( L"\\??\\BootDevice" ),
        RTL_CONSTANT_STRING( L"\\SYSTEM\\EXPLORER.EXE" ), OBJ_KERNEL_HANDLE };

    ZwWaitForSingleObject( 0, 2000 );

    PspCreateUserProcess( &explorer_Handle,
                          PROCESS_ALL_ACCESS,
                          &explorer_File );

    //
    // To be implemented: 
    // chapter 17 - debug junk
    // chapter 18 - performance monitoring
    //

    //KiRecursiveDirectoryList( &ObRootDirectory, 1 );

    //ZwWaitForSingleObject( 0, 8000 );
    //__debugbreak( );
}

EXTERN ULONG64 KxIntHandlerTable[ ];
KIDT_GATE BspIdt[ 256 ];

VOID
KiSystemStartup(
    _In_ PLDR_BOOT_INFO BootInfo
)
{

    HalEnableCpuFeatures( );

    KeRootSerial = BootInfo->RootSerial;
    PMEMORY_MAP MemoryMap = ( PMEMORY_MAP )( ULONG64 )( BootInfo->MemoryMap & 0xFFFF );
#if 0
    // idt is loaded pretty late into the boot sequence, this is some old stuff.
    KDESCRIPTOR_TABLE BspIdtr;
    for ( ULONG32 i = 0; i < 256; i++ ) {

        BspIdt[ i ].OffsetLow = KxIntHandlerTable[ i ];
        BspIdt[ i ].OffsetMid = KxIntHandlerTable[ i ] >> 16;
        BspIdt[ i ].OffsetHigh = KxIntHandlerTable[ i ] >> 32;
        BspIdt[ i ].Ist = 0;
        BspIdt[ i ].SegmentSelector = GDT_KERNEL_CODE64;
        BspIdt[ i ].Present = 1;
        BspIdt[ i ].Type = IDT_GATE_TYPE_INTERRUPT64;
    }
    BspIdtr.Base = ( ULONG64 )BspIdt;
    BspIdtr.Limit = sizeof( KIDT_GATE[ 256 ] ) - 1;
    __lidt( &BspIdtr );
#endif

    MmInitializeCaching( );
    MmInitializeMemoryManager( MemoryMap );
    ObInitializeObjectManager( );
    PsInitializeProcessSystem( );

    HalInitializeAcpi( );
    HalInitializeCpu0( );

    KiInitializeIpiCall( );
    HalStartProcessors( );
    KeInitializeKernelCore( );

    KIRQL PreviousIrql;
    KeRaiseIrql( DISPATCH_LEVEL, &PreviousIrql );

    OBJECT_ATTRIBUTES ThreadAttributes = { 0 };
    PKTHREAD Thread;
    PKPCB Processor;

    ObCreateObject( &Thread,
                    PsThreadObject,
                    &ThreadAttributes,
                    sizeof( KTHREAD ) );
    Thread->Header.Type = DPC_OBJECT_THREAD;
    Thread->StackLength = 0x20000;
    Thread->StackBase = ( ULONG64 )PspCreateStack( 0, Thread->StackLength );
    Thread->Process = PsInitialSystemProcess;
    Thread->ThreadId = KeGenerateUniqueId( );
    Thread->ProcessorNumber = PspGetThreadProcessor( );
    Thread->ExitCode = ( ULONG64 )-1;
    Thread->TrapFrame.Cr3 = __readcr3( );
    Thread->TrapFrame.Rsp = Thread->StackBase + Thread->StackLength - 0x28;
    *( ULONG64* )Thread->TrapFrame.Rsp = ( ULONG64 )PspSystemThreadReturn;
    Thread->TrapFrame.EFlags = 0x202;
    Thread->TrapFrame.Rip = ( ULONG64 )KiInitThread;
    Thread->TrapFrame.Rcx = ( ULONG64 )BootInfo;
    Thread->TrapFrame.SegCs = GDT_KERNEL_CODE64;
    Thread->TrapFrame.SegSs = GDT_KERNEL_DATA;
    Thread->TrapFrame.SegDs = GDT_KERNEL_DATA;
    Thread->TrapFrame.SegEs = GDT_KERNEL_DATA;
    Thread->TrapFrame.SegFs = GDT_KERNEL_DATA;
    Thread->TrapFrame.Dr6 = 0xFFFF0FF0;
    Thread->TrapFrame.Dr7 = 0x400;

    Processor = KeQueryProcessorByNumber( Thread->ProcessorNumber );
    Thread->TrapFrame.SegGs = Processor->SegGs;

    KeAcquireSpinLock( &Processor->ThreadQueueLock, &PreviousIrql );

    KeInsertTailList( &Processor->ThreadQueue->ThreadQueue, &Thread->ThreadQueue );
    Processor->ThreadQueueLength++;

    KeInsertTailList( PsInitialSystemProcess->ThreadLinks, &Thread->ThreadLinks );
    PsInitialSystemProcess->ThreadCount++;

    KeReleaseSpinLock( &Processor->ThreadQueueLock, PreviousIrql );

    Thread->ThreadState = THREAD_STATE_READY;

    while ( KiCpuInitComplete != HalLocalApicCount - 1 )
        ;

    KeReleaseSpinLockAtDpcLevel( &KiDispatcherLock );

    //RtlDebugPrint( L"my lil pogger %d\n", KiCpuInitComplete );

    KeLowerIrql( PASSIVE_LEVEL );
    while ( 1 ) {

        __halt( );
    }
}
