#pragma once

#include <carbsup.h>
#include "disk.h"
#include "controller.h"
#include "interface.h"
#include "partition.h"
#include "fat32.h"
#include "ntfs.h"

NTSTATUS
FsSplitDirectoryPath(
	__in PWCHAR DirectoryPath,
	__out PWCHAR** SplitPath
	);