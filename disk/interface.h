#pragma once

#define ATA_SR_BSY				0x80// Busy
#define ATA_SR_DRDY				0x40// Drive ready
#define ATA_SR_DF				0x20// Drive write fault
#define ATA_SR_DSC				0x10// Drive seek complete
#define ATA_SR_DRQ				0x08// Data request ready
#define ATA_SR_CORR				0x04// Corrected Value
#define ATA_SR_IDX				0x02// Index
#define ATA_SR_ERR				0x01// Status

#define ATA_ER_BBK				0x80// Bad block
#define ATA_ER_UNC				0x40// Uncorrectable Value
#define ATA_ER_MC				0x20// Media changed
#define ATA_ER_IDNF				0x10// ID mark not found
#define ATA_ER_MCR				0x08// Media change request
#define ATA_ER_ABRT				0x04// Command aborted
#define ATA_ER_TK0NF			0x02// Track 0 not found
#define ATA_ER_AMNF				0x01// No address mark
#if 0
#define ATA_REG_DATA			0x00
#define ATA_REG_ERROR			0x01
#define ATA_REG_FEATURES		0x01
#define ATA_REG_SECCOUNT0		0x02
#define ATA_REG_LBA0			0x03
#define ATA_REG_LBA1			0x04
#define ATA_REG_LBA2			0x05
#define ATA_REG_HDDEVSEL		0x06
#define ATA_REG_COMMAND			0x07
#define ATA_REG_STATUS			0x07
#define ATA_REG_SECCOUNT1		0x08
#define ATA_REG_LBA3			0x09
#define ATA_REG_LBA4			0x0A
#define ATA_REG_LBA5			0x0B
#define ATA_REG_CONTROL			0x0C
#define ATA_REG_ALTSTATUS		0x0C
#define ATA_REG_DEVADDRESS		0x0D
#endif

#define ATA_REG_DATA			0x00
#define ATA_REG_ERROR			0x01
#define ATA_REG_FEATURES		0x01
#define ATA_REG_SECCOUNT0		0x02
#define ATA_REG_LBA0			0x03
#define ATA_REG_LBA1			0x04
#define ATA_REG_LBA2			0x05
#define ATA_REG_HDDEVSEL		0x06
#define ATA_REG_COMMAND			0x07
#define ATA_REG_STATUS			0x07
#define ATA_REG_CONTROL			0x08


/*Table 116 - Command codes (sorted by command code) */
#define ATA_CMD_READ_PIO		0x20
#define ATA_CMD_READ_PIO_EXT	0x24
#define ATA_CMD_READ_DMA		0xC8	/*7.28 READ DMA - C8h, DMA */
#define ATA_CMD_READ_DMA_EXT	0x25	/*7.29 READ DMA EXT - 25h, DMA */
#define ATA_CMD_WRITE_PIO		0x30
#define ATA_CMD_WRITE_PIO_EXT	0x34
#define ATA_CMD_WRITE_DMA		0xCA
#define ATA_CMD_WRITE_DMA_EXT	0x35
#define ATA_CMD_CACHE_FLUSH		0xE7
#define ATA_CMD_CACHE_FLUSH_EXT	0xEA
#define ATA_CMD_PACKET			0xA0
#define ATA_CMD_IDENTIFY_PACKET	0xA1
#define ATA_CMD_IDENTIFY		0xEC

/*

	Minor and Major version numbers.

	7.17.7.40 Word 80: Major version number
		If not 0000h or FFFFh, the device claims compliance with the major version(s) as indicated by bits (6:3)
		being set to one. Values other than 0000h and FFFFh are bit significant. Since ATA standards maintain
		downward compatibility, a device may set more than one bit.
		7.17.7.41 Word 81: Minor version number
		If an implementor claims that the revision of the standard they used to guide their implementation does not
		need to be reported or if the implementation was based upon a standard prior to the ATA-3 standard, word
		81 shall be 0000h or FFFFh.
		X927HTable 13X defines the value that may optionally be reported in word 81 to indicate the revision of the standard
		that guided the implementation.

	Table 13 - Minor version number
		Value Minor revision
		0001h Obsolete
		0002h Obsolete
		0003h Obsolete
		0004h Obsolete
		0005h Obsolete
		0006h Obsolete
		0007h Obsolete
		0008h Obsolete
		0009h Obsolete
		000Ah Obsolete
		000Bh Obsolete
		000Ch Obsolete
		000Dh ATA/ATAPI-4 X3T13 1153D revision 6
		000Eh ATA/ATAPI-4 T13 1153D revision 13
		000Fh ATA/ATAPI-4 X3T13 1153D revision 7
		0010h ATA/ATAPI-4 T13 1153D revision 18
		0011h ATA/ATAPI-4 T13 1153D revision 15
		0012h ATA/ATAPI-4 published, ANSI INCITS 317-1998
		0013h ATA/ATAPI-5 T13 1321D revision 3
		0014h ATA/ATAPI-4 T13 1153D revision 14
		0015h ATA/ATAPI-5 T13 1321D revision 1
		0016h ATA/ATAPI-5 published, ANSI INCITS 340-2000
		0017h ATA/ATAPI-4 T13 1153D revision 17
		0018h ATA/ATAPI-6 T13 1410D revision 0
		0019h ATA/ATAPI-6 T13 1410D revision 3a
		001Ah ATA/ATAPI-7 T13 1532D revision 1
		001Bh ATA/ATAPI-6 T13 1410D revision 2
		001Ch ATA/ATAPI-6 T13 1410D revision 1
		001Dh ATA/ATAPI-7 published ANSI INCITS 397-2005.
		001Eh ATA/ATAPI-7 T13 1532D revision 0
		001Fh Reserved
		0020h Reserved
		0021h ATA/ATAPI-7 T13 1532D revision 4a
		0022h ATA/ATAPI-6 published, ANSI INCITS 361-2002
		0023h-FFFFh Reserved

*/


typedef struct _ATA_DEVICE_IDENTITY {

	union {

		struct {
			UCHAR Buffer[ 512 ];
		};

		struct {
			/*

				0 General configuration bit-significant information:
				F 15 0=ATA device
				F 14-8 Retired
				F 7 1=removable media device
				F 6 1=not removable controller and/or device
				F 5-3 Retired
				V 2 Response incomplete
				F 1 Retired
				F 0 Reserved
			*/

			/*
				Table 20 - IDENTIFY DEVICE information
			*/

			USHORT DeviceType;
			USHORT Cylinders;
			USHORT SpecificConfirguration;
			USHORT Heads;
			UCHAR Reserved[ 4 ];
			USHORT SectorsPerTrack;
			UCHAR Reserved1[ 6 ];
			UCHAR SerialNumber[ 20 ];
			UCHAR Reserved2[ 6 ];
			UCHAR FirmwareRevision[ 8 ];
			UCHAR ModelNumber[ 40 ];
			UCHAR Reserved3[ 4 ];
			/*
				49 Capabilities
					R 15-14 Reserved for the IDENTIFY PACKET DEVICE command.
					F 13 1=Standby timer values as specified in this standard are supported
					0=Standby timer values shall be managed by the device
					R 12 Reserved for the IDENTIFY PACKET DEVICE command.
					F 11 1=IORDY supported
					0=IORDY may be supported
					F 10 1=IORDY may be disabled
					R 9 Shall be set to one. Utilized by IDENTIFY PACKET DEVICE command.
					R 8 Shall be set to one. Utilized by IDENTIFY PACKET DEVICE command.
					X 7-0 Retired

				50 F Capabilities
					15 Shall be cleared to zero.
					14 Shall be set to one.
					13-1 Reserved.
					0 Shall be set to one to indicate a device specific Standby timer value
					minimum
			*/

			/*
			USHORT a, b;
			*/

			ULONG Capabilities;
			UCHAR Reserved4[ 6 ];
			USHORT CurrentLogicalCylinders;
			USHORT CurrentLogicalHeads;
			USHORT CurrentLogicalSectorsPerTrack;
			ULONG CurrentCapacityInSectors;
			UCHAR Reserved5[ 2 ];
			ULONG MaxLogicalBlockAddress;//0x0FFFFFFF
			UCHAR Reserved6[ 36 ];
			USHORT MajorVersionNumber;
			USHORT MinorVersionNumber;

			/*
				82 F Command set supported.
					15 Obsolete
					14 1=NOP command supported
					13 1=READ BUFFER command supported
					12 1=WRITE BUFFER command supported
					11 Obsolete
					10 1=Host Protected Area feature set supported
					9 1=DEVICE RESET command supported
					8 1=SERVICE interrupt supported
					7 1=release interrupt supported
					6 1=look-ahead supported
					5 1=write cache supported
					4 1=supports PACKET Command feature set
					3 1=supports Power Management feature set
					2 1=supports Removable Media feature set
					1 1=supports Security Mode feature set
					0 1=supports SMART feature set
				83 F Command sets supported.
					15 Shall be cleared to zero
					14 Shall be set to one
					13-9 Reserved
					8 1=SET MAX security extension supported
					7 Reserved for project 1407DT Address Offset Reserved Area Boot
					6 1=SET FEATURES subcommand required to spinup after power-up
					5 1=Power-Up In Standby feature set supported
					4 1=Removable Media Status Notification feature set supported
					3 1=Advanced Power Management feature set supported
					2 1=CFA feature set supported
					1 1=READ/WRITE DMA QUEUED supported
					0 1=DOWNLOAD MICROCODE command supported
					(continued)
					T13/1321D revision 2
					Page 93
					Table 20 - IDENTIFY DEVICE information (continued)
					Word F/V
				84 F Command set/feature supported extension.
					15 Shall be cleared to zero
					14 Shall be set to one
					13-0 Reserved
					85 V Command set/feature enabled.
					15 Obsolete
					14 1=NOP command enabled
					13 1=READ BUFFER command enabled
					12 1=WRITE BUFFER command enabled
					11 Obsolete
					10 1=Host Protected Area feature set enabled
					9 1=DEVICE RESET command enabled
					8 1=SERVICE interrupt enabled
					7 1=release interrupt enabled
					6 1=look-ahead enabled
					5 1=write cache enabled
					4 1= PACKET Command feature set enabled
					3 1= Power Management feature set enabled
					2 1= Removable Media feature set enabled
					1 1= Security Mode feature set enabled
					0 1= SMART feature set enabled
					86 V Command set/feature enabled.
					15-9 Reserved
					8 1=SET MAX security extension enabled by SET MAX SET PASSWORD
					7 Reserved for project 1407DT Address Offset Reserved Area Boot
					6 1=SET FEATURES subcommand required to spin-up after power-up
					5 1=Power-Up In Standby feature set enabled
					4 1=Removable Media Status Notification feature set enabled
					3 1=Advanced Power Management feature set enabled
					2 1=CFA feature set enabled
					1 1=READ/WRITE DMA QUEUED command supported
					0 1=DOWNLOAD MICROCODE command supported
				87 V Command set/feature default.
					15 Shall be cleared to zero
					14 Shall be set to one
					13-0 Reserved
			*/

			ULONG CommandSets1;
			ULONG CommandSets2;
			ULONG CommandSets3;

			UCHAR Reserved7[ 24 ];

			ULONG64 MaxLogicalBlockAddressExt; //&0x0000FFFFFFFFFFFF 


		};
	};

} ATA_DEVICE_IDENTITY, *PATA_DEVICE_IDENTITY;
