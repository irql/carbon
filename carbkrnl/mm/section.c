


#include <carbsup.h>
#include "mi.h"
#include "../hal/halp.h"
#include "../ke/ki.h"
#include "../rtl/ldr/ldrp.h"

//
// Notes on Section objects
//
// 1. Fix up FileObject & SectionObject's relationship,
//    especially in the MiCleanupSection procedure.
//
// 2. Implement an association of page attributes & physicals
//    for the mapview apis, and improve on loader functionality.
//    user mode will rely on these apis for image mapping because of
//    the integration with vads.
//

PMM_SECTION_CLUSTER
MiCreateSectionCluster(

)
{
    PMM_SECTION_CLUSTER Cluster;
    ULONG64 Address;

    Cluster = MmAllocatePoolWithTag( NonPagedPoolZeroed, sizeof( MM_SECTION_CLUSTER ), MM_TAG );

    for ( Address = 0; Address < 15; Address++ ) {

        Cluster->Address[ Address ] = ( ULONG64 )-1;
    }
    return Cluster;
}

VOID
MiInsertSectionAddress(
    _In_ PMM_SECTION_OBJECT SectionObject,
    _In_ ULONG64            Address,
    _In_ ULONG64            PageLength
)
{
    PMM_SECTION_CLUSTER AppendCluster;
    ULONG64 StartPage;
    ULONG64 CurrentPage;

    if ( SectionObject->FirstCluster == NULL ) {

        SectionObject->FirstCluster = MiCreateSectionCluster( );
        AppendCluster = SectionObject->FirstCluster;
    }
    else {

        AppendCluster = SectionObject->FirstCluster;
        while ( AppendCluster->Link != NULL ) {
            AppendCluster = AppendCluster->Link;
        }

        if ( AppendCluster->Address[ 14 ] != ( ULONG64 )-1 ) {

            AppendCluster->Link = MiCreateSectionCluster( );
            AppendCluster = AppendCluster->Link;
        }
    }

    for ( StartPage = 0; StartPage < 15; StartPage++ ) {

        if ( AppendCluster->Address[ StartPage ] == ( ULONG64 )-1 ) {
            break;
        }
    }

    for ( CurrentPage = 0; CurrentPage < PageLength; CurrentPage++ ) {

        if ( Address != ( ULONG64 )-1 ) {

            AppendCluster->Address[ StartPage + ( CurrentPage % 15 ) ] = Address + ( CurrentPage << 12 );
        }
        else {

            AppendCluster->Address[ StartPage + ( CurrentPage % 15 ) ] = MmAllocatePhysical( MmTypeSectionObject );
        }

        if ( StartPage + ( CurrentPage % 15 ) >= 14 ) {
            AppendCluster->Link = MiCreateSectionCluster( );
            AppendCluster = AppendCluster->Link;
            StartPage = 0;
        }
    }
}

VOID
MiRemoveSectionAddress(
    _In_ PMM_SECTION_OBJECT SectionObject,
    _In_ ULONG64            Index,
    _In_ ULONG64            Length,
    _In_ BOOLEAN            FreePhysical
)
{
    ULONG64 ClusterOffset;
    ULONG64 ClusterStartIndex;
    ULONG64 CurrentPage;
    PMM_SECTION_CLUSTER Current;
    PMM_SECTION_CLUSTER Link;

    if ( ( SectionObject->Length >> 12 ) < Index + Length ) {

        return;
    }

    ClusterOffset = Index / 15;
    ClusterStartIndex = Index % 15;

    Current = SectionObject->FirstCluster;
    while ( ClusterOffset-- ) {
        Current = Current->Link;
    }

    for ( CurrentPage = 0; CurrentPage < Length; CurrentPage++ ) {

        if ( FreePhysical ) {

            MmFreePhysical( Current->Address[ ClusterStartIndex + ( CurrentPage % 15 ) ] );
        }

        Current->Address[ ClusterStartIndex + ( CurrentPage % 15 ) ] = ( ULONG64 )-1;

        if ( ClusterStartIndex + ( CurrentPage % 15 ) >= 14 ) {
            Link = Current->Link;
            if ( ClusterStartIndex == 0 ) {
                MmFreePoolWithTag( Current, MM_TAG );
            }
            Current = Link;
            ClusterStartIndex = 0;
        }
    }
}

NTSTATUS
MmCreateSection(
    _Out_    PMM_SECTION_OBJECT* SectionObject,
    _In_opt_ POBJECT_ATTRIBUTES  ObjectAttributes,
    _In_     ULONG               AllocationAttributes,
    _In_opt_ PIO_FILE_OBJECT     FileObject
)
{
    //
    // Creates a section, the FileObject parameter is only
    // used for size and other misc stuff, the caller will
    // need to copy the associated FileObject data.
    //

    STATIC OBJECT_ATTRIBUTES SectionAttributes = { 0 };

    NTSTATUS ntStatus;
    PMM_SECTION_OBJECT Section;

    ntStatus = ObCreateObject( &Section,
                               MmSectionObject,
                               ObjectAttributes == NULL ? &SectionAttributes : ObjectAttributes,
                               sizeof( MM_SECTION_OBJECT ) );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    Section->FileObject = FileObject;

    if ( FileObject != NULL &&
        ( AllocationAttributes & SEC_NO_SHARE ) == 0 ) {

        FileObject->SectionObject = Section;
    }
    Section->Length = 0;//FileObject == NULL ? 0 : ROUND_TO_PAGES( FileObject->FileLength );
    Section->AllocationAttributes = AllocationAttributes & ( SEC_NOCACHE |
                                                             SEC_EXECUTE |
                                                             SEC_WRITE |
                                                             SEC_NO_EXTEND |
                                                             SEC_WRITECOMBINE |
                                                             SEC_IMAGE |
                                                             SEC_IMAGE_NO_EXECUTE |
                                                             SEC_UNINITIALIZED_FO |
                                                             SEC_NO_SHARE );

    *SectionObject = Section;
    return STATUS_SUCCESS;
}

NTSTATUS
MmCreateSectionSpecifyAddress(
    _Out_     PMM_SECTION_OBJECT* SectionObject,
    _In_opt_  POBJECT_ATTRIBUTES  ObjectAttributes,
    _In_      ULONG               AllocationAttributes,
    _In_      ULONG64             Address,
    _In_      ULONG64             Length
)
{
    STATIC OBJECT_ATTRIBUTES SectionAttributes = { 0 };

    NTSTATUS ntStatus;
    PMM_SECTION_OBJECT Section;

    ntStatus = ObCreateObject(
        &Section,
        MmSectionObject,
        ObjectAttributes == NULL ? &SectionAttributes : ObjectAttributes,
        sizeof( MM_SECTION_OBJECT ) );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    Section->Address = Address;
    Section->Length = ROUND_TO_PAGES( Length );
    Section->AllocationAttributes = ( AllocationAttributes | SEC_SPECIFY_ADDRESS ) & ( SEC_NOCACHE |
                                                                                       SEC_EXECUTE |
                                                                                       SEC_WRITE |
                                                                                       SEC_NO_EXTEND |
                                                                                       SEC_WRITECOMBINE |
                                                                                       SEC_SPECIFY_ADDRESS );

    MiInsertSectionAddress( Section, Section->Address, Section->Length >> 12 );

    *SectionObject = Section;
    return STATUS_SUCCESS;
}

NTSTATUS
MmResizeSection(
    _In_ PMM_SECTION_OBJECT SectionObject,
    _In_ ULONG64            SectionLength
)
{

    ULONG64 PageLength;
    ULONG64 PreviousPageLength;

    SectionLength = ROUND_TO_PAGES( SectionLength );
    PageLength = SectionLength >> 12;
    PreviousPageLength = SectionObject->Length >> 12;

    if ( ( SectionObject->AllocationAttributes & SEC_NO_EXTEND ) == SEC_NO_EXTEND ) {

        return STATUS_INVALID_ATTRIBUTES;
    }

    if ( SectionLength == SectionObject->Length ) {

        return STATUS_SUCCESS;
    }

    if ( ( SectionObject->AllocationAttributes & SEC_SPECIFY_ADDRESS ) == SEC_SPECIFY_ADDRESS ) {

        if ( SectionLength > SectionObject->Length ) {

            //
            // Size is being increased.
            //

            MiInsertSectionAddress(
                SectionObject,
                SectionObject->Address + SectionObject->Length,
                PageLength - PreviousPageLength );

            SectionObject->Length = SectionLength;

            return STATUS_SUCCESS;
        }
        else {

            //
            // Size is being decreased.
            //

            MiRemoveSectionAddress(
                SectionObject,
                PageLength - PreviousPageLength,
                PreviousPageLength - PageLength,
                FALSE );

            SectionObject->Length = SectionLength;

            return STATUS_SUCCESS;
        }
    }
    else {

        if ( SectionLength > SectionObject->Length ) {

            //
            // Size is being increased.
            //

            MiInsertSectionAddress(
                SectionObject,
                ( ULONG64 )-1,
                PageLength - PreviousPageLength );

            SectionObject->Length = SectionLength;

            return STATUS_SUCCESS;
        }
        else {

            //
            // Size is being decreased.
            //

            MiRemoveSectionAddress(
                SectionObject,
                PageLength - PreviousPageLength,
                PreviousPageLength - PageLength,
                TRUE );

            SectionObject->Length = SectionLength;

            return STATUS_SUCCESS;
        }
    }
}

PVOID
MiFindFreeUserRegion(
    _In_ PKPROCESS Process,
    _In_ ULONG64   PageLength
)
{
    BOOLEAN SecondChance;
    ULONG64 FoundLength;
    PVOID   FoundAddress;
    ULONG64 Level4;
    ULONG64 Level3;
    ULONG64 Level2;
    ULONG64 Level1;

    SecondChance = FALSE;
    FoundAddress = 0;
    FoundLength = PageLength;

    Level4 = MiIndexLevel4( Process->UserRegionHint );
    Level3 = MiIndexLevel3( Process->UserRegionHint );
    Level2 = MiIndexLevel2( Process->UserRegionHint );
    Level1 = MiIndexLevel1( Process->UserRegionHint );

SecondPass:;
    for ( ; Level4 < 256; Level4++ ) {

        if ( MiLevel4Table[ Level4 ].Present ) {
            for ( ; Level3 < 512; Level3++ ) {

                if ( MiReferenceLevel4Entry( Level4 )[ Level3 ].Present ) {
                    for ( ; Level2 < 512; Level2++ ) {

                        if ( MiReferenceLevel3Entry( Level4, Level3 )[ Level2 ].Present ) {
                            for ( ; Level1 < 512; Level1++ ) {

                                if ( MiReferenceLevel2Entry( Level4, Level3, Level2 )[ Level1 ].Present ) {

                                    FoundLength = PageLength;
                                    FoundAddress = 0;
                                }
                                else {
                                    if ( FoundAddress == 0 ) {
                                        FoundAddress = MiConstructAddress( Level4, Level3, Level2, Level1 );

                                        if ( ( ULONG64 )FoundAddress < 0x10000 ) {
                                            FoundAddress = 0;
                                            continue;
                                        }
                                    }

                                    FoundLength--;
                                    if ( FoundLength == 0 ) {
                                        Process->UserRegionHint = ( ULONG64 )FoundAddress + ( PageLength << 12 );
                                        return FoundAddress;
                                    }
                                }
                            }
                            Level1 = 0;
                        }
                        else {
                            if ( FoundAddress == 0 ) {
                                FoundAddress = MiConstructAddress( Level4, Level3, Level2, 0 );

                                if ( ( ULONG64 )FoundAddress < 0x10000 ) {
                                    FoundAddress = ( PVOID )0x10000;
                                }
                            }

                            if ( FoundLength <= 512 ) {
                                Process->UserRegionHint = ( ULONG64 )FoundAddress + ( PageLength << 12 );
                                return FoundAddress;
                            }
                            else {
                                FoundLength -= 512;
                            }
                        }
                    }
                    Level2 = 0;
                }
                else {
                    if ( FoundAddress == 0 ) {
                        FoundAddress = MiConstructAddress( Level4, Level3, 0, 0 );

                        if ( ( ULONG64 )FoundAddress < 0x10000 ) {
                            FoundAddress = ( PVOID )0x10000;
                        }
                    }

                    if ( FoundLength <= 512 * 512 ) {
                        Process->UserRegionHint = ( ULONG64 )FoundAddress + ( PageLength << 12 );
                        return FoundAddress;
                    }
                    else {
                        FoundLength -= 512 * 512;
                    }
                }
            }
            Level3 = 0;
        }
        else {
            if ( FoundAddress == 0 ) {
                FoundAddress = MiConstructAddress( Level4, 0, 0, 0 );

                if ( ( ULONG64 )FoundAddress < 0x10000 ) {
                    FoundAddress = ( PVOID )0x10000;
                }
            }

            if ( FoundLength <= 512 * 512 * 512 ) {
                Process->UserRegionHint = ( ULONG64 )FoundAddress + ( PageLength << 12 );
                return FoundAddress;
            }
            else {
                FoundLength -= 512 * 512 * 512;
            }
        }
    }

    Level4 = 0;
    Level3 = 0;
    Level2 = 0;
    Level1 = 0;

    if ( !SecondChance ) {
        SecondChance = TRUE;
        goto SecondPass;
    }

    return NULL;
}

BOOLEAN
MiIsRegionFree(
    _In_ PVOID     PageAddress,
    _In_ ULONG64   PageLength
)
{
    ULONG64 Level4;
    ULONG64 Level3;
    ULONG64 Level2;
    ULONG64 Level1;
    ULONG64 CurrentPage;

    for ( CurrentPage = 0; CurrentPage < PageLength; CurrentPage++ ) {

        Level4 = MiIndexLevel4( ( ULONG64 )PageAddress + ( CurrentPage << 12 ) );
        Level3 = MiIndexLevel3( ( ULONG64 )PageAddress + ( CurrentPage << 12 ) );
        Level2 = MiIndexLevel2( ( ULONG64 )PageAddress + ( CurrentPage << 12 ) );
        Level1 = MiIndexLevel1( ( ULONG64 )PageAddress + ( CurrentPage << 12 ) );

        if ( !MiLevel4Table[ Level4 ].Present ) {
            CurrentPage += 512 * 512 * 512;
            CurrentPage -= 1;
        }
        else {
            if ( !MiReferenceLevel4Entry( Level4 )[ Level3 ].Present ) {
                CurrentPage += 512 * 512;
                CurrentPage -= 1;
            }
            else {
                if ( !MiReferenceLevel3Entry( Level4, Level3 )[ Level2 ].Present ) {
                    CurrentPage += 512;
                    CurrentPage -= 1;
                }
                else {

                    if ( MiReferenceLevel2Entry( Level4, Level3, Level2 )[ Level1 ].Present ) {
                        return FALSE;
                    }
                }
            }
        }
    }

    return TRUE;
}

NTSTATUS
MmMapViewOfSection(
    _In_    PMM_SECTION_OBJECT SectionObject,
    _In_    PKPROCESS          Process,
    _Inout_ PVOID*             BaseAddress,
    _In_    ULONG64            ViewOffset,
    _In_    ULONG64            ViewLength,
    _In_    ULONG              Protection
)
{
    NTSTATUS ntStatus;
    ULONG64 PageLength;
    ULONG64 PageOffset;
    ULONG64 ClusterOffset;
    ULONG64 ClusterStartIndex;
    ULONG64 CurrentPage;
    ULONG64 PageAddress;
    PPMLE PageTable;
    MM_WSLE Entry;
    PMM_SECTION_CLUSTER Cluster;
    KIRQL PreviousIrql;
    PVOID FileBuffer;
    PIRP FileRequest;
    IO_STATUS_BLOCK FileStatus;
    ULONG64 LoaderLimit;
    PMM_VAD Vad;
    ULONG64 PreviousAddressSpace;
    PIMAGE_DOS_HEADER DosHeader;
    PIMAGE_NT_HEADERS NtHeaders;
    BOOLEAN LockHeld;

    LockHeld = FALSE;
    FileBuffer = NULL;
    PreviousAddressSpace = 0;

    //
    // We should force the caller to map a PE image,
    // or at least restrict these flags to kernel mode
    // only and have user code map user images.
    // We do need to allow file objects to be associated with section 
    // objects though.
    //

    if ( ( Protection & PAGE_READ ) != PAGE_READ ) {
        return STATUS_INVALID_ATTRIBUTES;
    }

    if ( ( Protection & PAGE_WRITE ) == PAGE_WRITE &&
        ( SectionObject->AllocationAttributes & SEC_WRITE ) != SEC_WRITE ) {
        return STATUS_INVALID_ATTRIBUTES;
    }

    if ( ( Protection & PAGE_EXECUTE ) == PAGE_EXECUTE &&
        ( SectionObject->AllocationAttributes & SEC_EXECUTE ) != SEC_EXECUTE ) {
        return STATUS_INVALID_ATTRIBUTES;
    }

    if ( SectionObject->AllocationAttributes & SEC_UNINITIALIZED_FO ) {

        if ( ( SectionObject->AllocationAttributes & SEC_IMAGE ) == SEC_IMAGE ) {

            FileBuffer = MmAllocatePoolWithTag( NonPagedPool, SectionObject->FileObject->FileLength, MM_TAG );

            FileRequest = IoBuildSynchronousFsdRequest( SectionObject->FileObject->DeviceObject,
                                                        IRP_MJ_READ,
                                                        FileBuffer,
                                                        SectionObject->FileObject->FileLength,
                                                        0u,
                                                        NULL,
                                                        &FileStatus );
            FileRequest->FileObject = SectionObject->FileObject;

            ntStatus = IoCallDriver( SectionObject->FileObject->DeviceObject, FileRequest );

            if ( !NT_SUCCESS( ntStatus ) ||
                 !NT_SUCCESS( FileStatus.Status ) ) {

                goto MiProcedureFinished;
            }

            ntStatus = LdrpGetLoaderLimits( FileBuffer,
                                            &LoaderLimit );
            if ( !NT_SUCCESS( ntStatus ) ) {

                goto MiProcedureFinished;
            }

            ntStatus = MmResizeSection( SectionObject, LoaderLimit );
            if ( !NT_SUCCESS( ntStatus ) ) {

                goto MiProcedureFinished;
            }
        }
        else {

            FileBuffer = MmAllocatePoolWithTag( NonPagedPool, SectionObject->FileObject->FileLength, MM_TAG );

            FileRequest = IoBuildSynchronousFsdRequest( SectionObject->FileObject->DeviceObject,
                                                        IRP_MJ_READ,
                                                        FileBuffer,
                                                        SectionObject->FileObject->FileLength,
                                                        0u,
                                                        NULL,
                                                        &FileStatus );
            FileRequest->FileObject = SectionObject->FileObject;

            ntStatus = IoCallDriver( SectionObject->FileObject->DeviceObject, FileRequest );

            if ( !NT_SUCCESS( ntStatus ) ||
                 !NT_SUCCESS( FileStatus.Status ) ) {

                goto MiProcedureFinished;
            }

            //
            // Resize the section and read it, after mapping it.
            //

            ntStatus = MmResizeSection( SectionObject, SectionObject->FileObject->FileLength );
            if ( !NT_SUCCESS( ntStatus ) ) {

                goto MiProcedureFinished;
            }
        }
    }

    if ( SectionObject->Length == 0 ) {

        ntStatus = STATUS_UNSUCCESSFUL;
        goto MiProcedureFinished;
    }

    if ( ViewLength == 0 ) {

        ViewLength = SectionObject->Length;
    }

    ViewLength = ROUND_TO_PAGES( ViewLength );
    PageLength = ViewLength >> 12;
    ViewOffset = ROUND_TO_PAGES( ViewOffset );
    PageOffset = ViewOffset >> 12;
    ClusterOffset = PageOffset / 15;
    ClusterStartIndex = PageOffset % 15;

    if ( SectionObject->Length - ViewOffset < ViewLength ) {

        ntStatus = STATUS_UNSUCCESSFUL;
        goto MiProcedureFinished;
    }

    if ( ( PreviousAddressSpace & ~0xFFF ) != ( Process->DirectoryTableBase & ~0xFFF ) ) {

        PreviousAddressSpace = MiGetAddressSpace( );
        MiSetAddressSpace( Process->DirectoryTableBase );
    }

    LockHeld = TRUE;
    if ( Process == PsInitialSystemProcess ) {

        KeAcquireSpinLock( &MmNonPagedPoolLock, &PreviousIrql );
    }
    else {

        KeAcquireSpinLock( &Process->UserRegionLock, &PreviousIrql );
    }

    if ( SectionObject->AllocationAttributes & SEC_UNINITIALIZED_FO &&
         SectionObject->AllocationAttributes & SEC_IMAGE ) {

        DosHeader = ( PIMAGE_DOS_HEADER )( ( ULONG64 )FileBuffer );

        if ( !LdrpCheckDos( DosHeader ) ) {

            ntStatus = STATUS_INVALID_IMAGE;
            goto MiProcedureFinished;
        }

        NtHeaders = ( PIMAGE_NT_HEADERS )( ( ULONG64 )FileBuffer + DosHeader->e_lfanew );

        if ( !LdrpCheckNt( NtHeaders ) ) {

            ntStatus = STATUS_INVALID_IMAGE;
            goto MiProcedureFinished;
        }

        if ( ( NtHeaders->FileHeader.Characteristics & IMAGE_FILE_DLL ) != IMAGE_FILE_DLL ) {

            //
            // if we're mapping an executable image and it is not a dll file, then
            // the file object should not be associated with this so that when ZwCreateSection
            // is called, the file is re-read and re-mapped
            //
            // The section object is okay to keep the file object pointer for recognition
            // purposes and other misc info.
            //

            SectionObject->FileObject->SectionObject = NULL;
            //SectionObject->FileObject = NULL;
        }

        if ( *BaseAddress != NULL ) {

            if ( ( ULONG64 )( *BaseAddress ) != NtHeaders->OptionalHeader.ImageBase &&
                 !LdrpIsRelocImage( NtHeaders ) ) {

                ntStatus = STATUS_INVALID_IMAGE;
                goto MiProcedureFinished;
            }
        }
        else if ( Process != PsInitialSystemProcess ) {

            *BaseAddress = ( PVOID )NtHeaders->OptionalHeader.ImageBase;
            if ( !MiIsRegionFree( *BaseAddress, PageLength ) ) {
                *BaseAddress = NULL;

                if ( !LdrpIsRelocImage( NtHeaders ) ) {

                    ntStatus = STATUS_INVALID_IMAGE;
                    goto MiProcedureFinished;
                }
            }
        }

    }

    if ( ( SectionObject->AllocationAttributes & SEC_IMAGE ) == SEC_IMAGE &&
        ( SectionObject->AllocationAttributes & SEC_UNINITIALIZED_FO ) != SEC_UNINITIALIZED_FO ) {

        //
        // Image bases are sometimes locked, if they have previously been mapped
        // this only applies if it's a dll file and the fo has been initialized.
        // The reason behind this is because of relocations.
        //
        // This eventually should allow for another section object to be created
        // if the base address is not available.
        //

        *BaseAddress = ( PVOID )SectionObject->LockedBase;

    }

    if ( *BaseAddress == NULL ) {

        if ( Process == PsInitialSystemProcess ) {

            PageAddress = ( ULONG64 )MiFindFreeNonPagedPoolRegion( PageLength );
        }
        else {

            PageAddress = ( ULONG64 )MiFindFreeUserRegion( Process, PageLength );
        }
    }
    else {
        *BaseAddress = ( PVOID )ROUND_TO_PAGES( *BaseAddress );

        if ( !MiIsRegionFree( *BaseAddress, PageLength ) ) {

            ntStatus = STATUS_INVALID_ADDRESS;
            goto MiProcedureFinished;
        }

        PageAddress = ( ULONG64 )*BaseAddress;
    }

    ObReferenceObject( SectionObject );

    Cluster = SectionObject->FirstCluster;
    while ( ClusterOffset-- ) {
        Cluster = Cluster->Link;
    }

    for ( CurrentPage = 0; CurrentPage < PageLength; CurrentPage++ ) {

        PageTable = MmAddressPageTable( PageAddress + ( CurrentPage << 12 ) );

        PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].PageFrameNumber = Cluster->Address[ ClusterStartIndex + ( CurrentPage % 15 ) ] >> 12;
        PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].Present = 1;

        if ( MiIndexLevel4( PageAddress + ( CurrentPage << 12 ) ) < 256 ) {

            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].User = 1;
        }

        if ( KeProcessorFeatureEnabled( KeQueryCurrentProcessor( ), KPF_NX_ENABLED ) ) {

            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].ExecuteDisable = ( Protection & PAGE_EXECUTE ) != PAGE_EXECUTE;
        }

        if ( Process == PsInitialSystemProcess ||
            ( SectionObject->AllocationAttributes & SEC_NO_SHARE ) == SEC_NO_SHARE ) {

            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].Write = ( Protection & PAGE_WRITE ) == PAGE_WRITE;
        }
        else {

            //
            // Because section objects are shared resources, it will be copied 
            // by the page fault handler when a process writes to it.
            //

            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].Write = FALSE;
        }

        if ( ( SectionObject->AllocationAttributes & SEC_NOCACHE ) == SEC_NOCACHE ) {
            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].Pat = 0;
            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].CacheDisable = 1;
            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].WriteThrough = 0; // Pa2
        }
        else if ( ( SectionObject->AllocationAttributes & SEC_WRITECOMBINE ) == SEC_WRITECOMBINE ) {
            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].Pat = 0;
            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].CacheDisable = 0;
            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].WriteThrough = 1; // Pa1
        }
        else {
            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].Pat = 0;
            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].CacheDisable = 0;
            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].WriteThrough = 0; // Pa0
        }

        if ( ClusterStartIndex + ( CurrentPage % 15 ) >= 14 ) {
            ClusterStartIndex = 0;
        }

        if ( ( CurrentPage % 15 ) >= 14 ) {
            Cluster = Cluster->Link;
        }
    }

    LockHeld = FALSE;
    if ( Process == PsInitialSystemProcess ) {

        KeReleaseSpinLock( &MmNonPagedPoolLock, PreviousIrql );
    }
    else {

        KeReleaseSpinLock( &Process->UserRegionLock, PreviousIrql );
    }

    Entry.Upper = 0;
    Entry.Lower = 0;
    Entry.Usage = MmMappedViewOfSection;
    Entry.ViewOfSection.SectionObject = ( ULONG64 )SectionObject;
    Entry.ViewOfSection.Length = PageLength;
    Entry.ViewOfSection.Address = PageAddress >> 12;
    Entry.ViewOfSection.NoCopy = ( Protection & PAGE_WRITE ) != PAGE_WRITE;

    KeAcquireSpinLock( &Process->WorkingSetLock, &PreviousIrql );
    MmInsertWorkingSet( &Entry );
    KeReleaseSpinLock( &Process->WorkingSetLock, PreviousIrql );

    if ( SectionObject->AllocationAttributes & SEC_UNINITIALIZED_FO ) {
        SectionObject->AllocationAttributes &= ~SEC_UNINITIALIZED_FO;

        if ( ( SectionObject->AllocationAttributes & SEC_IMAGE ) == SEC_IMAGE ) {

            //
            // Loader shit happens right here
            // you need to change this to only really use dlls? or static modules?
            // I think there is a flag on the PE images for this, but because we're 
            // sharing physical addresses when using section objects, we need to account 
            // for the fact that some files are not written to work like this.
            //
            // This is just a check to see if its a supervisor module or user mode one,
            // everything supervisor belongs in the system processes vadroot.
            //
            // TODO: Have the known dlls be loaded by this ONLY, the rest to be loaded
            //       by ntdll and injected into the PEB
            //

            Vad = MiAllocateVad( );
            Vad->FileObject = SectionObject->FileObject;

            SectionObject->LockedBase = PageAddress;

            if ( Process == PsInitialSystemProcess ) {

                ntStatus = LdrpLoadSupervisorModule( Process,
                                                     Vad,
                                                     ( PVOID )PageAddress,
                                                     PageLength << 12,
                                                     FileBuffer,
                                                     SectionObject->FileObject->FileLength );
                if ( !NT_SUCCESS( ntStatus ) ) {

                    MiFreeVad( Vad );
                    goto MiProcedureFinished;
                }
            }
            else {

                ntStatus = LdrpLoadUserModule( Process,
                                               Vad,
                                               ( PVOID )PageAddress,
                                               PageLength << 12,
                                               FileBuffer,
                                               SectionObject->FileObject->FileLength );
                if ( !NT_SUCCESS( ntStatus ) ) {

                    MiFreeVad( Vad );
                    goto MiProcedureFinished;
                }
            }

            //ObReferenceObject( SectionObject ); // vad reference
            // TODO: fix sect - file - vad refs
        }
        else {

            //RtlDebugPrint( L"Sect Map: %ull %ull %d\n", PageAddress, FileBuffer, SectionObject->FileObject->FileLength );
            RtlCopyMemory( ( PVOID )PageAddress, FileBuffer, SectionObject->FileObject->FileLength );
        }
    }
    else if ( ( SectionObject->AllocationAttributes & SEC_IMAGE ) == SEC_IMAGE ) {

        Vad = MiAllocateVad( );
        Vad->FileObject = SectionObject->FileObject;
        Vad->Start = SectionObject->LockedBase;
        Vad->Charge = SectionObject->Length;
        Vad->End = Vad->Start + Vad->Charge;

        MiInsertVad( Process, Vad );

        //
        // Any modules which are referenced by this must
        // be loaded too, we are going to traverse it's 
        // import table and load any of them.
        //

        if ( Process != PsInitialSystemProcess ) {

            //
            // We can safely assume that any
            // SystemProcess modules will be
            // loaded as the initial section 
            // into the address space
            //

            LdrpMapUserSection( Process,
                                Vad );

        }
    }

    *BaseAddress = ( PVOID )PageAddress;

    ntStatus = STATUS_SUCCESS;
MiProcedureFinished:;

    if ( LockHeld ) {

        if ( Process == PsInitialSystemProcess ) {

            KeReleaseSpinLock( &MmNonPagedPoolLock, PreviousIrql );
        }
        else {

            KeReleaseSpinLock( &Process->UserRegionLock, PreviousIrql );
        }
    }

    if ( FileBuffer != NULL ) {

        MmFreePoolWithTag( FileBuffer, MM_TAG );
    }

    if ( PreviousAddressSpace != 0 ) {

        MiSetAddressSpace( PreviousAddressSpace );
    }

    return ntStatus;
}

NTSTATUS
MmUnmapViewOfSection(
    _In_ PKPROCESS Process,
    _In_ PVOID     BaseAddress
)
{
    PMM_WSLE View;
    ULONG64  PageLength;
    ULONG64  CurrentPage;
    ULONG64  PageAddress;
    PPMLE    PageTable;
    PMM_SECTION_OBJECT Section;
    KIRQL PreviousIrql;

    if ( PsGetCurrentProcess( ) != Process ) {

        MiSetAddressSpace( Process->DirectoryTableBase );
    }

    KeAcquireSpinLock( &Process->WorkingSetLock, &PreviousIrql );

    View = MmFindWorkingSetByAddress( MmMappedViewOfSection, ( ULONG64 )BaseAddress );

    if ( View == NULL ) {

        KeReleaseSpinLock( &Process->WorkingSetLock, PreviousIrql );
        return STATUS_INVALID_ADDRESS;
    }

    PageAddress = ( ULONG64 )BaseAddress;
    PageLength = View->ViewOfSection.Length;
    Section = ( PMM_SECTION_OBJECT )( View->ViewOfSection.SectionObject | 0xFFFF000000000000 );

    if ( PageAddress > 0x800000000000 ) {
        KeAcquireSpinLock( &MmNonPagedPoolLock, &PreviousIrql );
    }

    for ( CurrentPage = 0; CurrentPage < PageLength; CurrentPage++ ) {

        PageTable = MmAddressPageTable( PageAddress + ( CurrentPage << 12 ) );

        if ( View->ViewOfSection.Copied ) {

            MmFreePhysical( PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].PageFrameNumber << 12 );
            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].Long = 0;
        }
        else {

            PageTable[ MiIndexLevel1( PageAddress + ( CurrentPage << 12 ) ) ].Long = 0;
        }
    }

    if ( PageAddress > 0x800000000000 ) {
        KeReleaseSpinLock( &MmNonPagedPoolLock, PreviousIrql );
    }

    for ( CurrentPage = 0; CurrentPage < PageLength; CurrentPage++ ) {
        MmFlushAddress( ( PVOID )( PageAddress + ( CurrentPage << 12 ) ) );
    }

    MmFreeWorkingSetListEntry( View );
    ObDereferenceObject( Section );

    KeReleaseSpinLock( &Process->WorkingSetLock, PreviousIrql );

    if ( PsGetCurrentProcess( ) != Process ) {

        MiSetAddressSpace( PsGetCurrentProcess( )->DirectoryTableBase );
    }

    return STATUS_SUCCESS;
}

NTSTATUS
ZwCreateSection(
    _Out_    PHANDLE            SectionHandle,
    _In_     ACCESS_MASK        DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_     ULONG              AllocationAttributes,
    _In_opt_ HANDLE             FileHandle
)
{

    NTSTATUS ntStatus;
    PMM_SECTION_OBJECT SectionObject;
    PIO_FILE_OBJECT FileObject;
    ACCESS_MASK HandleAccess;

    *SectionHandle = 0;
    FileObject = NULL;

    if ( ARGUMENT_PRESENT( FileHandle ) ) {
        HandleAccess = GENERIC_READ;

        if ( ( AllocationAttributes & SEC_WRITE ) == SEC_WRITE ) {

            HandleAccess |= GENERIC_WRITE;
        }

        if ( ( AllocationAttributes & SEC_EXECUTE ) == SEC_EXECUTE ) {

            HandleAccess |= GENERIC_EXECUTE;
        }

        ntStatus = ObReferenceObjectByHandle( &FileObject,
                                              FileHandle,
                                              HandleAccess,
                                              KernelMode,
                                              IoFileObject );
        if ( !NT_SUCCESS( ntStatus ) ) {

            return ntStatus;
        }

        //
        // IMPORTANT TODO: Fix up section object & file object attributes and associations
        //
    }

    AllocationAttributes = AllocationAttributes & ( SEC_NOCACHE |
                                                    SEC_EXECUTE |
                                                    SEC_WRITE |
                                                    SEC_WRITECOMBINE |
                                                    SEC_IMAGE |
                                                    SEC_IMAGE_NO_EXECUTE |
                                                    SEC_NO_SHARE );

    if ( ARGUMENT_PRESENT( FileHandle ) ) {

        AllocationAttributes |= SEC_UNINITIALIZED_FO;
    }

    if ( FileObject != NULL && FileObject->SectionObject != NULL &&
        ( AllocationAttributes & SEC_NO_SHARE ) == 0 ) {

        SectionObject = FileObject->SectionObject;
        ObReferenceObject( SectionObject );
    }
    else {

        ntStatus = MmCreateSection( &SectionObject,
                                    ObjectAttributes,
                                    AllocationAttributes,
                                    FileObject );
        if ( !NT_SUCCESS( ntStatus ) ) {

            goto done;
        }
    }

    ntStatus = ObOpenObjectFromPointer( SectionHandle,
                                        SectionObject,
                                        DesiredAccess,
                                        ObjectAttributes->Attributes,
                                        KernelMode );
    if ( !NT_SUCCESS( ntStatus ) ) {

        goto done;
    }

done:;

    if ( !NT_SUCCESS( ntStatus ) ) {

        if ( *SectionHandle != 0 ) {

            ZwClose( *SectionHandle );
        }

        return ntStatus;
    }

    ObDereferenceObject( SectionObject );

    if ( FileObject != NULL ) {

        // FileObject reference.
        //ObDereferenceObject( FileObject );
    }

    return STATUS_SUCCESS;
}

NTSTATUS
ZwResizeSection(
    _In_ HANDLE  SectionHandle,
    _In_ ULONG64 SectionLength
)
{
    NTSTATUS ntStatus;
    PMM_SECTION_OBJECT SectionObject;

    ntStatus = ObReferenceObjectByHandle( &SectionObject,
                                          SectionHandle,
                                          SECTION_EXTEND_SIZE,
                                          KernelMode,
                                          MmSectionObject );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    ntStatus = MmResizeSection( SectionObject, SectionLength );
    ObDereferenceObject( SectionObject );
    return ntStatus;
}

NTSTATUS
ZwMapViewOfSection(
    _In_  HANDLE  SectionHandle,
    _In_  HANDLE  ProcessHandle,
    _Out_ PVOID*  BaseAddress,
    _In_  ULONG64 ViewOffset,
    _In_  ULONG64 ViewLength,
    _In_  ULONG   Protection
)
{
    NTSTATUS ntStatus;
    PKPROCESS Process;
    PMM_SECTION_OBJECT Section;
    ACCESS_MASK HandleAccess;

    if ( ( Protection & PAGE_READ ) != PAGE_READ ) {

        return STATUS_INVALID_ATTRIBUTES;
    }

    ntStatus = ObReferenceObjectByHandle( &Process,
                                          ProcessHandle,
                                          PROCESS_VM_READ | PROCESS_VM_WRITE,
                                          KernelMode,
                                          PsProcessObject );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    HandleAccess = SECTION_MAP_READ;

    if ( ( Protection & PAGE_WRITE ) == PAGE_WRITE ) {

        HandleAccess |= SECTION_MAP_WRITE;
    }

    if ( ( Protection & PAGE_EXECUTE ) == PAGE_EXECUTE ) {

        HandleAccess |= SECTION_MAP_EXECUTE;
    }

    ntStatus = ObReferenceObjectByHandle( &Section,
                                          SectionHandle,
                                          HandleAccess,
                                          KernelMode,
                                          MmSectionObject );
    if ( !NT_SUCCESS( ntStatus ) ) {

        ObDereferenceObject( Process );
        return ntStatus;
    }

    ntStatus = MmMapViewOfSection( Section,
                                   Process,
                                   BaseAddress,
                                   ViewOffset,
                                   ViewLength,
                                   Protection );

    ObDereferenceObject( Section );
    ObDereferenceObject( Process );
    return ntStatus;
}

NTSTATUS
ZwUnmapViewOfSection(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID  BaseAddress
)
{
    NTSTATUS ntStatus;
    PKPROCESS Process;

    ntStatus = ObReferenceObjectByHandle( &Process,
                                          ProcessHandle,
                                          PROCESS_VM_READ | PROCESS_VM_WRITE,
                                          KernelMode,
                                          PsProcessObject );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    ntStatus = MmUnmapViewOfSection( Process, BaseAddress );
    ObDereferenceObject( Process );
    return ntStatus;
}

VOID
MiCleanupSection(
    _In_ PMM_SECTION_OBJECT Section
)
{
    if ( ( Section->AllocationAttributes & SEC_SPECIFY_ADDRESS ) != SEC_SPECIFY_ADDRESS ) {

        MiRemoveSectionAddress( Section, 0, Section->Length >> 12, TRUE );
    }

    if ( Section->AllocationAttributes & ( SEC_IMAGE | SEC_IMAGE_NO_EXECUTE ) ) {

        ObDereferenceObject( Section->FileObject );
    }
}

NTSTATUS
NtCreateSection(
    _Out_    PHANDLE            SectionHandle,
    _In_     ACCESS_MASK        DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_     ULONG              AllocationAttributes,
    _In_opt_ HANDLE             FileHandle
)
{
    __try {
        return ZwCreateSection( SectionHandle,
                                DesiredAccess,
                                ObjectAttributes,
                                AllocationAttributes,
                                FileHandle );
    }
    __except ( EXCEPTION_EXECUTE_HANDLER ) {

        return STATUS_UNSUCCESSFUL;
    }
}

NTSTATUS
NtMapViewOfSection(
    _In_  HANDLE  SectionHandle,
    _In_  HANDLE  ProcessHandle,
    _Out_ PVOID*  BaseAddress,
    _In_  ULONG64 ViewOffset,
    _In_  ULONG64 ViewLength,
    _In_  ULONG   Protection
)
{
    __try {
        return ZwMapViewOfSection( SectionHandle,
                                   ProcessHandle,
                                   BaseAddress,
                                   ViewOffset,
                                   ViewLength,
                                   Protection );
    }
    __except ( EXCEPTION_EXECUTE_HANDLER ) {

        return STATUS_UNSUCCESSFUL;
    }
}

NTSTATUS
NtUnmapViewOfSection(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID  BaseAddress
)
{
    __try {
        return ZwUnmapViewOfSection( ProcessHandle,
                                     BaseAddress );
    }
    __except ( EXCEPTION_EXECUTE_HANDLER ) {

        return STATUS_UNSUCCESSFUL;
    }
}

NTSTATUS
NtResizeSection(
    _In_ HANDLE  SectionHandle,
    _In_ ULONG64 SectionLength
)
{
    __try {
        return ZwResizeSection( SectionHandle,
                                SectionLength );
    }
    __except ( EXCEPTION_EXECUTE_HANDLER ) {

        return STATUS_UNSUCCESSFUL;
    }
}
