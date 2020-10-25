


#pragma once

VOID
FsIdeDetectDrives(

	);

VOID
FsAhciDetectDrives(

	);


#include "interface.h"

#define DIRECTION_READ	0x00
#define DIRECTION_WRITE 0x01

typedef struct _IDE_DISK_DATA *PIDE_DISK_DATA;
typedef struct _AHCI_DISK_DATA *PAHCI_DISK_DATA;



