


#pragma once

#define SVGA_TAG 'AGVS'
#include <carbsup.h>
#include "../../corerd/pci/pci.h"
#include "../dxgi/dxgi.h"
#include "../dxgi/ddi/ddi.h"

#define DdGetDevice( device ) ( (PSVGA_DEVICE)( device )->DeviceExtension )
