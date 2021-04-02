


#include <carbusr.h>

#define ID_LIST_OBJECT      0x1000
#define ID_LIST_DIRECTORY   0x1001
#define ID_EDIT_ADDRESS_BAR 0x1002
#define ID_BTN_UP           0x1003

HANDLE ExpListObject;
HANDLE ExpListDirectory;
HANDLE ExpAddressBar;
HANDLE ExpButtonUp;
WCHAR  ExpInternalCd[ 512 ];

VOID
ExpClearListDirectory(

)
{
    ULONG64 Index;
    PWSTR Name;
    WND_PROC Procedure;

    NtGetWindowProc( ExpListDirectory, &Procedure );

    Index = Procedure( ExpListDirectory,
                       LV_GETITEMCOUNT,
                       0,
                       0 );

    while ( Index-- ) {

        Name = ( ( PLV_ITEM )Procedure( ExpListDirectory,
                                        LV_GETITEM,
                                        Index,
                                        0 ) )->Name;
        Procedure( ExpListDirectory,
                   LV_REMOVEITEM,
                   Index,
                   0 );
        RtlFreeHeap( RtlGetCurrentHeap( ),
                     Name );
    }
}

VOID
ExpInsertList(
    _In_ HANDLE List,
    _In_ PWSTR  Name
)
{
    WND_PROC Procedure;
    PWSTR NameDuplicate;

    NtGetWindowProc( List, &Procedure );

    NameDuplicate = RtlAllocateHeap( RtlGetCurrentHeap( ),
                                     wcslen( Name ) * sizeof( WCHAR ) + sizeof( WCHAR ) );
    wcscpy( NameDuplicate, Name );

    Procedure( List,
               LV_INSERTITEM,
               ( ULONG64 )NameDuplicate,
               0 );
}

VOID
ExpUpdateList(
    _In_ HANDLE List
)
{
    NtSendMessage( List,
                   WM_PAINT,
                   0,
                   0 );
}

NTSTATUS
ExpSetCurrentDirectory(
    _In_ PWSTR Directory
)
{
    NTSTATUS ntStatus;
    HANDLE FileHandle;
    IO_STATUS_BLOCK StatusBlock;
    PFILE_DIRECTORY_INFORMATION Info;
    UCHAR Buffer[ 512 ];
    ULONG64 FileIndex;

    Info = ( PFILE_DIRECTORY_INFORMATION )Buffer;
    FileIndex = 0;

    FileHandle = CreateFileW( Directory,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              FILE_OPEN_IF,
                              0 );

    if ( FileHandle == INVALID_HANDLE_VALUE ) {

        return STATUS_UNSUCCESSFUL;
    }

    ExpClearListDirectory( );

    do {

        ntStatus = NtQueryDirectoryFile( FileHandle,
                                         &StatusBlock,
                                         Info,
                                         512,
                                         FileDirectoryInformation,
                                         NULL,
                                         FileIndex,
                                         TRUE );
        if ( !NT_SUCCESS( ntStatus ) || !NT_SUCCESS( StatusBlock.Status ) ) {

            break;
        }

        ExpInsertList( ExpListDirectory,
                       Info->FileName );
        FileIndex++;

    } while ( TRUE );

    wcscpy( ExpInternalCd, Directory );

    NtDefaultWindowProc( ExpAddressBar,
                         WM_SETTEXT,
                         ( ULONG64 )Directory,
                         256 );
    ExpUpdateList( ExpAddressBar );
    ExpUpdateList( ExpListDirectory );

    return STATUS_SUCCESS;
}

PLV_ITEM
ExpGetList(
    _In_ HANDLE  List,
    _In_ ULONG64 Index
)
{
    WND_PROC Procedure;

    NtGetWindowProc( List, &Procedure );

    return ( PLV_ITEM )Procedure( List,
                                  LV_GETITEM,
                                  Index,
                                  0 );
}

ULONG64
ExpGetSelectedId(
    _In_ HANDLE  List
)
{
    WND_PROC Procedure;

    NtGetWindowProc( List, &Procedure );

    return ( ULONG64 )Procedure( List,
                                 LV_GETSELECTED,
                                 0,
                                 0 );
}

NTSTATUS
ExMessageProcedure(
    _In_ HANDLE  WindowHandle,
    _In_ ULONG32 MessageId,
    _In_ ULONG64 Param1,
    _In_ ULONG64 Param2
)
{
    //NtClassWinBaseProc

    ULONG64 IdSel;
    WCHAR Buffer[ 256 ];

    switch ( MessageId ) {
    case WM_COMMAND:;

        switch ( Param1 ) {
        case ID_LIST_OBJECT:;

            if ( Param2 == LV_SELECTED ) {
                IdSel = ExpGetSelectedId( ExpListObject );

                if ( IdSel != -1 ) {

                    ExpSetCurrentDirectory( ExpGetList( ExpListObject, IdSel )->Name );
                    //ExpUpdateList( ExpListObject );
                }
            }

            break;
        case ID_LIST_DIRECTORY:;

            if ( Param2 == LV_PRESSED ) {
                IdSel = ExpGetSelectedId( ExpListDirectory );

                if ( IdSel != -1 ) {

                    wcscpy( Buffer, ExpInternalCd );
                    wcscat( Buffer, ExpGetList( ExpListDirectory, IdSel )->Name );
                    // check if its a directory :^)
                    wcscat( Buffer, L"\\" );
                    ExpSetCurrentDirectory( Buffer );
                }
            }

            break;
        case ID_EDIT_ADDRESS_BAR:;
            if ( Param2 == ED_ENTER ) {

                NtDefaultWindowProc( ExpAddressBar,
                                     WM_GETTEXT,
                                     ( ULONG64 )Buffer,
                                     256 );

                ExpSetCurrentDirectory( Buffer );
            }

            break;
        default:
            break;
        }
    default:
        return NtClassWinBaseProc( WindowHandle,
                                   MessageId,
                                   Param1,
                                   Param2 );
    }

}


VOID
NtProcessStartup(

)
{
    HANDLE WindowHandle;

    WND_CLASS Class;
    WND_PROC Procedure;
    KUSER_MESSAGE Message;

    LdrInitializeProcess( );

    Class.WndProc = ExMessageProcedure;
    wcscpy( Class.ClassName, L"ClassExplorer" );

    NtRegisterClass( &Class );

    NtCreateWindow( &WindowHandle,
                    INVALID_HANDLE_VALUE,
                    L"File Explorer",
                    L"ClassExplorer",
                    800,
                    500,
                    650,
                    400,
                    0 );
    NtCreateWindow( &ExpButtonUp,
                    WindowHandle,
                    L"",
                    L"BUTTON",
                    5,
                    25,
                    23,
                    23,
                    ID_BTN_UP );
    NtCreateWindow( &ExpAddressBar,
                    WindowHandle,
                    L"C:\\",
                    L"EDIT",
                    5 + 23 + 5,
                    25,
                    650 - 5 - 5 - 23 - 5,
                    23,
                    ID_EDIT_ADDRESS_BAR );
    NtCreateWindow( &ExpListObject,
                    WindowHandle,
                    L"ExpListObject",
                    L"LISTVIEW",
                    5,
                    25 + 23 + 5,
                    150,
                    400 - 25 - 5 - 23 - 5,
                    ID_LIST_OBJECT );
    NtCreateWindow( &ExpListDirectory,
                    WindowHandle,
                    L"ExpListDiectory",
                    L"LISTVIEW",
                    5 + 150 + 5,
                    25 + 23 + 5,
                    500 - 5 - 5 - 5,
                    400 - 25 - 5 - 23 - 5,
                    ID_LIST_DIRECTORY );

    while ( TRUE ) {

        NtWaitMessage( WindowHandle );

        if ( NtReceiveMessage( WindowHandle, &Message ) ) {

            NtGetWindowProc( WindowHandle, &Procedure );
            Procedure( WindowHandle,
                       Message.MessageId,
                       Message.Param1,
                       Message.Param2 );
        }

        if ( NtReceiveMessage( ExpListObject, &Message ) ) {

            NtGetWindowProc( ExpListObject, &Procedure );
            Procedure( ExpListObject,
                       Message.MessageId,
                       Message.Param1,
                       Message.Param2 );

            if ( Message.MessageId == WM_ACTIVATE ) {

                ExpInsertList( ExpListObject, L"C:\\" );
                ExpInsertList( ExpListObject, L"BootDevice\\" );
                Procedure( ExpListObject,
                           LV_SETSELECTED,
                           0,
                           0 );
            }
        }

        if ( NtReceiveMessage( ExpListDirectory, &Message ) ) {

            NtGetWindowProc( ExpListDirectory, &Procedure );
            Procedure( ExpListDirectory,
                       Message.MessageId,
                       Message.Param1,
                       Message.Param2 );
            if ( Message.MessageId == WM_ACTIVATE ) {

                ExpSetCurrentDirectory( L"C:\\" );
            }
        }

        if ( NtReceiveMessage( ExpAddressBar, &Message ) ) {

            NtGetWindowProc( ExpAddressBar, &Procedure );
            Procedure( ExpAddressBar,
                       Message.MessageId,
                       Message.Param1,
                       Message.Param2 );
        }

        if ( NtReceiveMessage( ExpButtonUp, &Message ) ) {

            NtGetWindowProc( ExpButtonUp, &Procedure );
            Procedure( ExpButtonUp,
                       Message.MessageId,
                       Message.Param1,
                       Message.Param2 );
        }
    }

}
