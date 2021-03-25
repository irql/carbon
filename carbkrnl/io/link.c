


#include <carbsup.h>
#include "iop.h"

NTSTATUS
IoCreateSymbolicLink(
    _In_ PUNICODE_STRING LinkName,
    _In_ PUNICODE_STRING LinkTarget
)
{
    NTSTATUS ntStatus;
    OBJECT_ATTRIBUTES ObjectAttributes = { 0 };
    POBJECT_SYMBOLIC_LINK Link;

    ObjectAttributes.Attributes = OBJ_PERMANENT_OBJECT;
    RtlCopyMemory( &ObjectAttributes.ObjectName, LinkName, sizeof( UNICODE_STRING ) );

    ntStatus = ObCreateObject( &Link, IoSymbolicLinkObject, &ObjectAttributes, sizeof( OBJECT_SYMBOLIC_LINK ) );
    if ( !NT_SUCCESS( ntStatus ) ) {

        return ntStatus;
    }

    RtlCopyMemory( &Link->LinkTarget, LinkTarget, sizeof( UNICODE_STRING ) );

    return STATUS_SUCCESS;
}