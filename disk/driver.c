



#include "driver.h"

NTSTATUS
DriverEntry(
	__in PDRIVER_OBJECT DriverObject
);

NTSTATUS
DriverUnload(
	__in PDRIVER_OBJECT DriverObject
);

NTSTATUS
DriverDispatch(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp
);

NTSTATUS
DriverEntry(
	__in PDRIVER_OBJECT DriverObject
)
{

	DriverObject->DriverUnload = DriverUnload;

	NTSTATUS ntStatus;
	UNICODE_STRING DiskLink = RTL_CONSTANT_UNICODE_STRING( L"\\Harddisk" );
	PDEVICE_OBJECT DeviceObject;

	ntStatus = IoCreateDevice( DriverObject, &DiskLink, &DeviceObject );

	if ( !NT_SUCCESS( ntStatus ) ) {

		return ntStatus;
	}

	for ( ULONG32 i = 0; i < IRP_MJ_MAX; i++ ) {

		DeviceObject->MajorFunction[ i ] = DriverDispatch;
	}

	FsIdeDetectDrives( );
	FsAhciDetectDrives( );

	return STATUS_SUCCESS;
}

NTSTATUS
DriverUnload(
	__in PDRIVER_OBJECT DriverObject
)
{
	DriverObject;

	return STATUS_SUCCESS;
}

NTSTATUS
DriverDispatch(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp
)
{
	DeviceObject;

	Irp->UserIosb.Information = 0;
	Irp->UserIosb.Status = STATUS_INVALID_REQUEST;

	switch ( Irp->StackLocation->MajorFunction ) {
	case IRP_MJ_CREATE: {

		NTSTATUS ntStatus;
		PWCHAR* SplitPath = NULL;

		PWCHAR DirectoryPath = ( PWCHAR )ExAllocatePoolWithTag( Irp->FileObject->FileName->MaximumLength, 'htaP' );
		_memcpy( ( void* )DirectoryPath, ( void* )Irp->FileObject->FileName->Buffer, ( int )Irp->FileObject->FileName->MaximumLength );

		ULONG32 FsDirectoryPathIndex = 0;

		ntStatus = FsSplitDirectoryPath( DirectoryPath, &SplitPath );

		if ( !NT_SUCCESS( ntStatus ) )
			goto IrpCreateDone;

		if ( SplitPath[ 0 ] == NULL )
			goto IrpCreateDone;

		if ( wcsncmp( SplitPath[ 0 ], L"PhysicalDrive", 13 ) != 0 )
			goto IrpCreateDone;

		if ( SplitPath[ 0 ][ 13 ] < '0' || SplitPath[ 0 ][ 13 ] > '9' )
			goto IrpCreateDone;

		Irp->FileObject->DiskId = wtoi( &SplitPath[ 0 ][ 13 ] );
		FsDirectoryPathIndex += ( _wcslen( SplitPath[ 0 ] ) + 1 );

		if ( SplitPath[ 1 ] == NULL )
			goto IrpCreateDone;//handle to raw disk

		if ( wcsncmp( SplitPath[ 1 ], L"Partition", 9 ) != 0 )
			goto IrpCreateDone;

		if ( SplitPath[ 1 ][ 9 ] < '0' || SplitPath[ 1 ][ 9 ] > '9' )
			goto IrpCreateDone;

		Irp->FileObject->PartitionId = wtoi( &SplitPath[ 1 ][ 9 ] );
		FsDirectoryPathIndex += ( _wcslen( SplitPath[ 1 ] ) + 1 );

		RtlAllocateAndInitUnicodeStringEx( &Irp->FileObject->FsFileName, ( PWSTR )( Irp->FileObject->FileName->Buffer + FsDirectoryPathIndex ) );

		PDISK_OBJECT Drive = FsGetDiskById( Irp->FileObject->DiskId );

		if ( Drive == NULL ) {
			ntStatus = STATUS_INVALID_PATH;

			goto IrpCreateDone;
		}

		PPARTITION_OBJECT Partition = FsGetPartitionById( Drive, Irp->FileObject->PartitionId );

		if ( Partition == NULL ) {
			ntStatus = STATUS_INVALID_PATH;

			goto IrpCreateDone;
		}

		Partition->FileSystem.Read( Partition, Irp->FileObject, &Irp->UserIosb );

	IrpCreateDone:;
		ExFreePoolWithTag( DirectoryPath, 'htaP' );

		if ( SplitPath != NULL ) {

			ExFreePoolWithTag( SplitPath, 'htaP' );
		}

		Irp->UserIosb.Information = 0;
		Irp->UserIosb.Status = ntStatus;

		break;
	}
	case IRP_MJ_READ: {

		if ( ( ULONG64 )Irp->StackLocation->Parameters.Read.Length > Irp->FileObject->FileMemorySize )
			Irp->StackLocation->Parameters.Read.Length = ( ULONG32 )Irp->FileObject->FileMemorySize;

		_memcpy( Irp->SystemBuffer, ( void* )( ( PCHAR )Irp->FileObject->FileMemory + Irp->StackLocation->Parameters.Read.ByteOffset ), Irp->StackLocation->Parameters.Read.Length );

		Irp->UserIosb.Information = Irp->StackLocation->Parameters.Read.Length;
		Irp->UserIosb.Status = STATUS_SUCCESS;

		break;
	}
	case IRP_MJ_QUERY_INFORMATION_FILE: {

		if ( Irp->StackLocation->Parameters.QueryFile.FileInformationClass == FileBasicInformation ) {

			if ( Irp->StackLocation->Parameters.QueryFile.Length != sizeof( FILE_BASIC_INFORMATION ) ) {

				Irp->UserIosb.Information = sizeof( FILE_BASIC_INFORMATION );
				Irp->UserIosb.Status = STATUS_INVALID_BUFFER;

				return STATUS_SUCCESS;
			}

			PFILE_BASIC_INFORMATION BasicInfo = ( PFILE_BASIC_INFORMATION )Irp->SystemBuffer;

			BasicInfo->FileSizeOnDisk = Irp->FileObject->FileMemorySize;
			BasicInfo->FileSize = Irp->FileObject->FileSize;

			Irp->UserIosb.Information = sizeof( FILE_BASIC_INFORMATION );
			Irp->UserIosb.Status = STATUS_SUCCESS;
		}

		//...
		break;
	}
	case IRP_MJ_CLOSE: {
#if 0
		NTSTATUS ntStatus;
		PDISK_OBJECT Disk = FsGetDiskById( Irp->FileObject->DiskId );

		if ( Disk == NULL ) {
			ntStatus = STATUS_INVALID_PATH;

			goto IrpCloseDone;
		}

		PPARTITION_OBJECT Partition = FsGetPartitionById( Disk, Irp->FileObject->PartitionId );

		if ( Partition == NULL ) {
			ntStatus = STATUS_INVALID_PATH;

			goto IrpCloseDone;
		}

		Partition->FileSystem.Write( Partition, Irp->FileObject, &Irp->UserIosb );

	IrpCloseDone:;

		Irp->UserIosb.Information = 0;
		Irp->UserIosb.Status = ntStatus;
#endif
		break;
	}
	default:
		break;
	}

	return STATUS_SUCCESS;
}