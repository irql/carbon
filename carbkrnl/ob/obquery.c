


#include <carbsup.h>
#include "obp.h"

NTSTATUS
ZwOpenDirectoryObject(
    _Out_ PHANDLE            DirectoryHandle,
    _In_  ACCESS_MASK        DesiredAccess,
    _In_  POBJECT_ATTRIBUTES ObjectAttributes
)
{
    DirectoryHandle;
    DesiredAccess;
    ObjectAttributes;
    NTSTATUS ntStatus;
    POBJECT_DIRECTORY ObjectDirectory;

    ntStatus = ObReferenceObjectByName( &ObjectDirectory,
                                        &ObjectAttributes->ObjectName,
                                        ObDirectoryObject );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    ntStatus = ObOpenObjectFromPointer( DirectoryHandle,
                                        ObjectDirectory,
                                        DesiredAccess,
                                        ObjectAttributes->Attributes,
                                        KernelMode );
    ObDereferenceObject( ObjectDirectory );

    return ntStatus;
}

NTSTATUS
ZwQueryDirectoryObject(
    _In_      HANDLE   DirectoryHandle,
    _Out_     PVOID    Buffer,
    _In_      ULONG64  Length,
    _Out_opt_ PULONG64 ReturnLength
)
{
    NTSTATUS ntStatus;
    POBJECT_DIRECTORY ObjectDirectory;

    ntStatus = ObReferenceObjectByHandle( &ObjectDirectory,
                                          DirectoryHandle,
                                          0,
                                          KernelMode,
                                          ObDirectoryObject );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    struct {
        POBJECT_TYPE    Type;
        PUNICODE_STRING Name;
    } *NameList;

    ULONG64 RequiredLength;
    ULONG64 StringLength;
    ULONG64 EntryCount;
    ULONG64 TypeCount;
    ULONG64 CurrentType;

    ULONG64 CurrentNameListLength;

    CurrentNameListLength = 64 / sizeof( *NameList );
    NameList = MmAllocatePoolWithTag( NonPagedPoolZeroed, 64, OB_TAG );

    RequiredLength = 0;
    TypeCount = 0;
    EntryCount = 0;

    while ( ObjectDirectory != NULL ) {

        RequiredLength += ( ULONG64 )ObjectDirectory->Name.Length + 2;
        RequiredLength += sizeof( OBJECT_DIRECTORY_INFORMATION );
        EntryCount++;

        for ( CurrentType = 0; CurrentType < TypeCount; CurrentType++ ) {

            if ( NameList[ CurrentType ].Type ==
                 ObpGetHeaderFromObject( ObjectDirectory->Object )->Type ) {

                break;
            }
        }

        if ( NameList[ CurrentType ].Type !=
             ObpGetHeaderFromObject( ObjectDirectory->Object )->Type ) {


            //NameList[ TypeCount ]
        }


        ObjectDirectory = ObjectDirectory->DirectoryLink;
    }

    ReturnLength;
    Length;
    StringLength;
    Buffer;

    return STATUS_SUCCESS;
}
