


#include "carbon.h"
#include "kd.h"
#include "kdp.h"
#include "pdb.h"
#include "osl/osl.h"

//
// Buffer must be of size sizeof(KD_PACKET) + Length
//

KD_STATUS
KdpReadDebuggee(
    _In_  ULONG64 Address,
    _In_  ULONG64 Length,
    _Out_ PVOID   Buffer
)
{
    KD_STATUS Status;
    CHAR PacketBuffer[ sizeof( KD_PACKET ) + 0x1000 ];
    PKD_PACKET Packet = ( PKD_PACKET )&PacketBuffer;
    //_alloca( sizeof( KD_PACKET ) + min( Length, 0x1000 ) );//OslAllocate( sizeof( KD_PACKET ) + Length );//

    do {

        struct {
            ULONG64 Process;
            ULONG64 Address;
            ULONG64 Length;
        } ReadSend;

        ReadSend.Process = ( ULONG64 )KdpProcess;
        ReadSend.Address = Address;
        if ( Length > 0x1000 ) {
            ReadSend.Length = 0x1000;
        }
        else {
            ReadSend.Length = Length;
        }
#if 0
        //KdSendPacket( KdPacketRead, &ReadSend, sizeof( ReadSend ) );
        OslSendPacket( KdPacketRead, &ReadSend, sizeof( ReadSend ) );
        while ( OslReceivePacket( KdPacketAcknowledge, Packet, sizeof( KD_PACKET ) ) != KdStatusSuccess )
            ;

        while ( OslReceivePacket( KdPacketRead, Packet, sizeof( KD_PACKET ) + ( ULONG32 )Length ) != KdStatusSuccess )
            ;
        OslSendPacket( KdPacketAcknowledge, NULL, 0 );
#else
        Status = KdSendPacketResponse( KdPacketRead,
                                       &ReadSend,
                                       sizeof( ReadSend ),
                                       KdPacketRead,
                                       Packet,
                                       sizeof( KD_PACKET ) + 0x1000 );
        if ( Status != KdStatusSuccess ) {

            OslWriteConsole( L"failed to read addr=%p status=%d length=%d process=%p\n",
                             ReadSend.Address, Status, ReadSend.Length, ReadSend.Process );
            //__debugbreak( );
            return Status;
        }
#endif

        if ( Length > 0x1000 ) {

            RtlCopyMemory( Buffer, &Packet->u[ 0 ].PacketRead.Return.Data, 0x1000 );
            Length -= 0x1000;
        }
        else {

            RtlCopyMemory( Buffer, &Packet->u[ 0 ].PacketRead.Return.Data, Length );
            Length = 0;
        }

        Buffer = ( PUCHAR )Buffer + 0x1000;
        Address += 0x1000;

    } while ( Length != 0 );

    return KdStatusSuccess;
}

KD_STATUS
KdpWriteDebuggee(
    _In_ ULONG64 Address,
    _In_ ULONG64 Length,
    _In_ PVOID   Buffer
)
{
    struct {
        ULONG64 Process;
        ULONG64 Address;
        UCHAR   Data[ 0 ];
    } *WriteSend = ( decltype( WriteSend ) )_alloca( 16 + Length );//OslAllocate( 8 + Length );//

    WriteSend->Process = ( ULONG64 )KdpProcess;
    WriteSend->Address = Address;
    RtlCopyMemory( &WriteSend->Data[ 0 ], Buffer, Length );

    return KdSendPacket( KdPacketWrite, WriteSend, 16 + ( ULONG32 )Length );
}

PKPROCESS
KdpGetThreadProcess(
    _In_ PKTHREAD Thread
)
{
    STATIC LONG ProcessOffset = 0;
    ULONG64 ProcessAddress;


    if ( ProcessOffset == 0 ) {

        DbgFieldOffset( KdpKernelContext, L"_KTHREAD", L"Process", &ProcessOffset );
    }

    return KdpReadDebuggee( ( ULONG64 )Thread + ProcessOffset,
                            sizeof( ULONG64 ),
                            &ProcessAddress ) == KdStatusSuccess ? ( PKPROCESS )ProcessAddress : NULL;
}

ULONG64
KdpGetThreadId(
    _In_ PKTHREAD Thread
)
{
    STATIC LONG ThreadIdOffset = 0;
    ULONG64 ThreadId;


    if ( ThreadIdOffset == 0 ) {

        DbgFieldOffset( KdpKernelContext, L"_KTHREAD", L"ThreadId", &ThreadIdOffset );
    }

    return KdpReadDebuggee( ( ULONG64 )Thread + ThreadIdOffset,
                            sizeof( ULONG64 ),
                            &ThreadId ) == KdStatusSuccess ? ThreadId : NULL;
}


//
// Process should be a remote address
//

BOOLEAN
KdpFindVadByAddress(
    _In_    PKPROCESS Process,
    _In_    ULONG64   Address,
    _Inout_ PMM_VAD   Vad
)
{
    STATIC LONG VadRootOffset = 0;
    ULONG64 VadAddress;

    if ( VadRootOffset == 0 ) {

        DbgFieldOffset( KdpKernelContext, L"_KPROCESS", L"VadRoot", &VadRootOffset );
    }

    //
    // Use the Vad argument as a temporary buffer to store the VAD
    //

    VadAddress = KdpReadULong64( ( ULONG64 )Process + VadRootOffset );

    do {

        KdpReadDebuggee( VadAddress, sizeof( MM_VAD ), Vad );

        if ( Address >= Vad->Start &&
             Address < Vad->End ) {

            return TRUE;
        }

        VadAddress = ( ULONG64 )Vad->Link;
    } while ( Vad->Link != NULL );

    return FALSE;
}

BOOLEAN
KdpFindVadByShortName(
    _In_    PKPROCESS Process,
    _In_    PWCHAR    ShortName,
    _Inout_ PMM_VAD   Vad
)
{
    STATIC LONG VadRootOffset = 0;
    ULONG64 VadAddress;
    WCHAR NameBuffer[ 256 ];
    WCHAR ShortNameBuffer[ 256 ];

    if ( VadRootOffset == 0 ) {

        DbgFieldOffset( KdpKernelContext, L"_KPROCESS", L"VadRoot", &VadRootOffset );
    }

    VadAddress = KdpReadULong64( ( ULONG64 )Process + VadRootOffset );

    do {

        KdpReadDebuggee( VadAddress, sizeof( MM_VAD ), Vad );

        KdpGetVadFileName( Vad, NameBuffer, NULL );
        KdpGetShortName( NameBuffer, ShortNameBuffer );

        if ( lstrcmpiW( ( PCWSTR )ShortName, ( PCWSTR )ShortNameBuffer ) == 0 ) {

            return TRUE;
        }

        VadAddress = ( ULONG64 )Vad->Link;
    } while ( Vad->Link != NULL );

    return FALSE;
}

//
// A little inconsistent with other apis, but this 
// one expects the vad to be read into the debugger
// address space.
//

VOID
KdpGetVadFileName(
    _In_      PMM_VAD Vad,
    _Out_     PVOID   Buffer,
    _Out_opt_ PUSHORT RequiredLength
)
{
    UNICODE_STRING String;

    KdpReadDebuggee( ( ULONG64 )Vad->FileObject + FIELD_OFFSET( IO_FILE_OBJECT, FileName ), sizeof( UNICODE_STRING ), &String );

    if ( RequiredLength != NULL ) {

        *RequiredLength = String.Length + 2;
    }

    if ( Buffer != NULL ) {

        KdpReadDebuggee( ( ULONG64 )String.Buffer, String.Length, Buffer );
        *( ( PUCHAR )Buffer + String.Length ) = 0;
    }
}

BOOLEAN
KdpReadDataDirectory(
    _In_      ULONG64  ModuleStart,
    _In_      ULONG64  Directory,
    _In_opt_  PVOID    Buffer,
    _Out_opt_ PULONG64 RequiredLength
)
{
    PIMAGE_DOS_HEADER HeaderDos;
    PIMAGE_NT_HEADERS HeadersNt;
    IMAGE_DATA_DIRECTORY DataDirectory;

    HeaderDos = ( PIMAGE_DOS_HEADER )( ModuleStart );
    HeadersNt = ( PIMAGE_NT_HEADERS )( ModuleStart + KdpReadULong32( ModuleStart + FIELD_OFFSET( IMAGE_DOS_HEADER, e_lfanew ) ) );

    KdpReadDebuggee( ( ULONG64 )HeadersNt + FIELD_OFFSET( IMAGE_NT_HEADERS, OptionalHeader.DataDirectory[ Directory ] ),
                     sizeof( IMAGE_DATA_DIRECTORY ),
                     &DataDirectory );

    if ( DataDirectory.VirtualAddress == 0 ||
         DataDirectory.Size == 0 ) {

        return FALSE;
    }

    if ( RequiredLength != NULL ) {

        *RequiredLength = ( ULONG64 )DataDirectory.Size;
    }

    if ( Buffer != NULL ) {

        KdpReadDebuggee( ModuleStart + DataDirectory.VirtualAddress, DataDirectory.Size, Buffer );
    }

    return TRUE;
}

VOID
KdpGetThreadStack(
    _In_      PKTHREAD Thread,
    _Out_opt_ PULONG64 StackBase,
    _Out_opt_ PULONG64 StackLength
)
{
    STATIC LONG StackBaseOffset = 0;
    STATIC LONG StackLengthOffset = 0;
    STATIC LONG SyscallActiveOffset = 0;
    STATIC LONG KernelStackBaseOffset = 0;
    STATIC LONG KernelStackLengthOffset = 0;

    //
    // If we start unwinding and unwind the KiFastSystemCall function
    // then this function will tell KdpUnwindFrame the incorrect stack 
    // range, to make things easier I made another api to get both 
    // stacks.
    //

    if ( StackBaseOffset == 0 ) {

        DbgFieldOffset( KdpKernelContext, L"_KTHREAD", L"StackBase", &StackBaseOffset );
        DbgFieldOffset( KdpKernelContext, L"_KTHREAD", L"StackLength", &StackLengthOffset );
        DbgFieldOffset( KdpKernelContext, L"_KTHREAD", L"Syscall", &SyscallActiveOffset );
        DbgFieldOffset( KdpKernelContext, L"_KTHREAD", L"KernelStackBase", &KernelStackBaseOffset );
        DbgFieldOffset( KdpKernelContext, L"_KTHREAD", L"KernelStackLength", &KernelStackLengthOffset );
    }

    if ( KdpReadULong64( ( ULONG64 )Thread + SyscallActiveOffset ) & 1 ) {

        if ( StackBase != NULL ) {

            KdpReadDebuggee( ( ULONG64 )Thread + KernelStackBaseOffset, sizeof( ULONG64 ), StackBase );
        }

        if ( StackLength != NULL ) {

            KdpReadDebuggee( ( ULONG64 )Thread + KernelStackLengthOffset, sizeof( ULONG64 ), StackLength );
        }
    }
    else {

        if ( StackBase != NULL ) {

            KdpReadDebuggee( ( ULONG64 )Thread + StackBaseOffset, sizeof( ULONG64 ), StackBase );
        }

        if ( StackLength != NULL ) {

            KdpReadDebuggee( ( ULONG64 )Thread + StackLengthOffset, sizeof( ULONG64 ), StackLength );
        }
    }
}

VOID
KdpGetThreadStackEx(
    _In_      PKTHREAD Thread,
    _Out_opt_ PULONG64 StackBase,
    _Out_opt_ PULONG64 StackLength,
    _Out_opt_ PULONG64 KernelStackBase,
    _Out_opt_ PULONG64 KernelStackLength
)
{
    //
    // TODO !!!!: this relies on legacy stacks, use the PspQueryStack method
    //

    STATIC LONG StackBaseOffset = 0;
    STATIC LONG StackLengthOffset = 0;
    STATIC LONG KernelStackBaseOffset = 0;
    STATIC LONG KernelStackLengthOffset = 0;

    if ( StackBaseOffset == 0 ) {

        DbgFieldOffset( KdpKernelContext, L"_KTHREAD", L"StackBase", &StackBaseOffset );
        DbgFieldOffset( KdpKernelContext, L"_KTHREAD", L"StackLength", &StackLengthOffset );
        DbgFieldOffset( KdpKernelContext, L"_KTHREAD", L"KernelStackBase", &KernelStackBaseOffset );
        DbgFieldOffset( KdpKernelContext, L"_KTHREAD", L"KernelStackLength", &KernelStackLengthOffset );
    }


    if ( KernelStackBase != NULL ) {

        KdpReadDebuggee( ( ULONG64 )Thread + KernelStackBaseOffset, sizeof( ULONG64 ), KernelStackBase );
    }

    if ( KernelStackLength != NULL ) {

        KdpReadDebuggee( ( ULONG64 )Thread + KernelStackLengthOffset, sizeof( ULONG64 ), KernelStackLength );
    }


    if ( StackBase != NULL ) {

        KdpReadDebuggee( ( ULONG64 )Thread + StackBaseOffset, sizeof( ULONG64 ), StackBase );
    }

    if ( StackLength != NULL ) {

        KdpReadDebuggee( ( ULONG64 )Thread + StackLengthOffset, sizeof( ULONG64 ), StackLength );
    }

}

EXTERN_C
ULONG64
KdpReadULong64(
    _In_ ULONG64 Address
)
{
    ULONG64 Long;
    KdpReadDebuggee( Address, sizeof( ULONG64 ), &Long );

    return Long;
}

EXTERN_C
VOID
KdpWriteULong64(
    _In_ ULONG64 Address,
    _In_ ULONG64 Long
)
{

    KdpWriteDebuggee( Address, sizeof( ULONG64 ), &Long );
}

EXTERN_C
ULONG32
KdpReadULong32(
    _In_ ULONG64 Address
)
{
    ULONG32 Long;
    KdpReadDebuggee( Address, sizeof( ULONG32 ), &Long );

    return Long;
}

EXTERN_C
VOID
KdpWriteULong32(
    _In_ ULONG64 Address,
    _In_ ULONG32 Long
)
{

    KdpWriteDebuggee( Address, sizeof( ULONG32 ), &Long );
}

EXTERN_C
USHORT
KdpReadUShort(
    _In_ ULONG64 Address
)
{
    USHORT Long;
    KdpReadDebuggee( Address, sizeof( USHORT ), &Long );

    return Long;
}

EXTERN_C
VOID
KdpWriteUShort(
    _In_ ULONG64 Address,
    _In_ USHORT  Long
)
{

    KdpWriteDebuggee( Address, sizeof( USHORT ), &Long );
}

EXTERN_C
USHORT
KdpReadUChar(
    _In_ ULONG64 Address
)
{
    UCHAR Long;
    KdpReadDebuggee( Address, sizeof( UCHAR ), &Long );

    return Long;
}

EXTERN_C
VOID
KdpWriteUChar(
    _In_ ULONG64 Address,
    _In_ UCHAR   Long
)
{

    KdpWriteDebuggee( Address, sizeof( UCHAR ), &Long );
}

VOID
KdpHandleGuestException(
    _In_ PKD_PACKET Exception
)
{
#if 0
    STATIC CONST wchar_t* Exception[ ] = {
        L"div by zero",
        L"debug",
        L"nmi",
        L"breakpoint",
        L"overflow",
        L"bound range exceeded",
        L"invalid opcode",
        L"device not available",
        L"double fault",
        L"???",
        L"invalid tss",
        L"segment not present",
        L"stack segment fault",
        L"general protection fault",
        L"page fault",
        L"???",
        L"x87 fpu exception",
        L"alignment check",
        L"machine check",
        L"simd fpu exception",
        L"virtualization exception",
        L"???",
        L"???",
        L"???",
        L"???",
        L"???",
        L"???",
        L"???",
        L"???",
        L"security exception"
        L"???"
    };
#endif
    HRESULT hResult;
    NTSTATUS ntStatus;
    PWCHAR FileName;
    PWCHAR FunctionName;
    LONG   FunctionDisplacement;
    ULONG  LineNumber;

    PKPROCESS Process;
    PKTHREAD Thread;

    PKD_CACHED_MODULE Cached;

    OslDelayExecution( 250 );

    Thread = Exception->u[ 0 ].PacketException.Initial.Record.Thread;
    Process = KdpGetThreadProcess( Thread );

    KdpProcess = Process;

    OslWriteConsole( L"\n** EXCEPTION %d **\n"
                     L"RIP=%p RSP=%p TID=%d\n"
                     L"STACK FRAME:\n",
                     Exception->u[ 0 ].PacketException.Initial.Record.Status,
                     Exception->u[ 0 ].PacketException.Initial.Record.ExceptionContext.Rip,
                     Exception->u[ 0 ].PacketException.Initial.Record.ExceptionContext.Rsp,
                     KdpGetThreadId( Thread ) );

    do {

        Cached = KdpGetModuleByAddress( Exception->u[ 0 ].PacketException.Initial.Record.ExceptionContext.Rip );

        if ( Cached == NULL ) {

            OslWriteConsole( L"              0x%p\n",
                             Exception->u[ 0 ].PacketException.Initial.Record.ExceptionContext.Rip );
        }
        else {

            hResult = DbgGetFunctionByAddress( Cached,
                                               Exception->u[ 0 ].PacketException.Initial.Record.ExceptionContext.Rip - Cached->Start,
                                               &FunctionName,
                                               &FileName,
                                               &LineNumber,
                                               &FunctionDisplacement );

            if ( hResult != 0 ) {

                OslWriteConsole( L"  %30s 0x%p (0x%08x)\n",
                                 Cached->Name,
                                 Exception->u[ 0 ].PacketException.Initial.Record.ExceptionContext.Rip,
                                 Exception->u[ 0 ].PacketException.Initial.Record.ExceptionContext.Rip - Cached->Start );
            }
            else {
                OslWriteConsole( L"  %30s %48s:%04d %s+%d\n", Cached->Name,
                                 FileName, LineNumber, FunctionName, FunctionDisplacement );
            }
        }

        ntStatus = KdpUnwindFrame( Exception->u[ 0 ].PacketException.Initial.Record.Thread,
                                   &Exception->u[ 0 ].PacketException.Initial.Record.ExceptionContext );
    } while ( NT_SUCCESS( ntStatus ) );

    OslWriteConsole( L"\n" );
}

VOID
KdpInputThread(

)
{
    BOOLEAN    JustBroke;
    BOOLEAN    Acknowledge;
    PKD_PACKET Packet = ( PKD_PACKET )OslAllocate( 0x4000 );//_alloca( 4096 );
   // ULONG      SystemProcessRva;

    //OslDelayExecution( 350 ); // wait for the os to just be ready before we start sending & retrying commands.
#if 0
    DbgGetAddressByName( KdpKernelContext,
                         L"PsInitialSystemProcess",
                         &SystemProcessRva );
    KdpProcess = ( PVOID )KdpReadULong64( 0xFFFFFFFFFFE00000 + SystemProcessRva );
#endif
    JustBroke = FALSE;

    OslWriteConsole( L"PsInitialSystemProcess: %p\n", KdpProcess );

    while ( 1 ) {

        //
        // Small hack to make sure if an exception occurred or there is
        // some sort of info for this break, it doesn't release the lock
        // just yet, and processes it first.
        //

        if ( !JustBroke ) {

            OslAcquireCommunicationLock( );
        }
        else {

            JustBroke = FALSE;
        }

        if ( OslReceivePacket( KdPacketAny, Packet, 4096 ) == KdStatusSuccess ) {

            Acknowledge = TRUE;
            switch ( Packet->PacketType ) {
            case KdPacketConnect:
                break;
            case KdPacketException:
                OslSendPacket( KdPacketAcknowledge, NULL, 0 );
                Acknowledge = FALSE;

                KdpHandleGuestException( Packet );
                break;
            case KdPacketBreakIn:
                OslWriteConsole( L"break\n" );
                OslSignalBrokeIn( );
                JustBroke = TRUE;
                break;
            case KdPacketContinue:
                OslWriteConsole( L"continued\n" );
                OslSignalContinue( );
                break;
            case KdPacketPrint:
                OslWriteConsole( ( PCWCHAR )&Packet->u[ 0 ].PacketPrint.Initial.String );
                break;
            default:
                Acknowledge = FALSE;
                break;
            }

            if ( Acknowledge ) {

                OslSendPacket( KdPacketAcknowledge, NULL, 0 );
            }
        }

        if ( !JustBroke ) {

            OslReleaseCommunicationLock( );
        }

        OslDelayExecution( 50 );
    }
}
