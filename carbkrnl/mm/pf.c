


#include <carbsup.h>
#include "../hal/halp.h"
#include "mi.h"

NTSTATUS
MiPageFaultHandle(
    _In_ PKTRAP_FRAME TrapFrame,
    _In_ ULONG64      Address
)
{
    TrapFrame;

    //
    // change this to COW only a single page at a time
    //

    PMM_WSLE AddressList;
    ULONG64 PageLength;
    ULONG64 PageCurrent;
    ULONG64 PageAddress;
    PPMLE PageTable;
    PVOID PageIntermediate;
    PPMLE PageIntermedateLevel2;

    if ( TrapFrame->Error != 0x2 ) {

        return STATUS_UNSUCCESSFUL;
    }

    AddressList = MmFindWorkingSetByAddress( MmMappedViewOfSection,
                                             Address );

    if ( AddressList == NULL ) {

        return STATUS_UNSUCCESSFUL;
    }

    //
    // AddressList is the section which the process page faulted
    // on, we can now implement copy-on-write and re-map this
    // resource.
    //
    // TODO !!!!!!: if a file is mapped as a section resource in memory
    // then the I/O manager should not allow this file to be written to.
    // section objects must be mapped from a FILE_SHARE_WRITE disabled
    // handle.
    //

    if ( !AddressList->ViewOfSection.Copied &&
         !AddressList->ViewOfSection.NoCopy ) {

        PageIntermediate = MmMapIoSpaceSpecifyCache( 0, 0x1000, MmCacheWriteBack );
        PageIntermedateLevel2 = MiReferenceLevel2Entry( MiIndexLevel4( PageIntermediate ),
                                                        MiIndexLevel3( PageIntermediate ),
                                                        MiIndexLevel2( PageIntermediate ) );


        AddressList->ViewOfSection.Copied = 1;
        PageLength = AddressList->ViewOfSection.Length;
        PageAddress = AddressList->ViewOfSection.Address << 12;

        for ( PageCurrent = 0; PageCurrent < PageLength; PageCurrent++ ) {

            PageTable = MmAddressPageTable( PageAddress + ( PageCurrent << 12 ) );

            PageIntermedateLevel2[ MiIndexLevel1( PageIntermediate ) ].PageFrameNumber =
                PageTable[ MiIndexLevel1( PageAddress + ( PageCurrent << 12 ) ) ].PageFrameNumber;

            PageTable[ MiIndexLevel1( PageAddress + ( PageCurrent << 12 ) ) ].PageFrameNumber =
                MmAllocatePhysical( MmTypeProcessPrivate ) >> 12;
            PageTable[ MiIndexLevel1( PageAddress + ( PageCurrent << 12 ) ) ].Write = TRUE;

            RtlCopyMemory( ( PVOID )( PageAddress + ( PageCurrent << 12 ) ),
                           PageIntermediate,
                           0x1000 );
        }

        MmUnmapIoSpace( PageIntermediate );

        RtlDebugPrint( L"Performed a copy-on-write for %s\n",
            ( ( PMM_SECTION_OBJECT )( AddressList->ViewOfSection.SectionObject | 0xFFFF000000000000 ) )->FileObject->FileName.Buffer );
    }

    return STATUS_SUCCESS;
}
