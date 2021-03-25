


#pragma once

#define DXGI_TAG 'igxd'

#include "../../core/ntuser/usersup.h"
#include "dxstatus.h"

EXTERN PDRIVER_OBJECT DdiDriverObject;

#define IOCTL_GET_VERSION       0x00
#define IOCTL_GET_CAPS          0x01
#define IOCTL_SURFACE_CREATE    0x02
#define IOCTL_SURFACE_DESTROY   0x03
#define IOCTL_CONTEXT_CREATE    0x04
#define IOCTL_CONTEXT_DESTROY   0x05
#define IOCTL_SET_RENDER_TARGET 0x06
#define IOCTL_SET_RENDER_STATE  0x07
#define IOCTL_SET_TEXTURE_STATE 0x08
#define IOCTL_SET_TRANSFORM     0x09
#define IOCTL_SET_MATERIAL      0x0A
#define IOCTL_SET_LIGHT_DATA    0x0B
#define IOCTL_SET_LIGHT_ENABLED 0x0C
#define IOCTL_SHADER_CREATE     0x0D
#define IOCTL_SHADER_DESTROY    0x0E
#define IOCTL_PRESENT           0x0F
#define IOCTL_CLEAR             0x10
#define IOCTL_BLT               0x11
#define IOCTL_STRETCH_BLT       0x12
#define IOCTL_SET_ZRANGE        0x13
#define IOCTL_SET_VIEWPORT      0x14
#define IOCTL_SUBMIT_COMMAND    0x15
