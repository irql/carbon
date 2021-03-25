


#pragma once

#pragma pack( 8 )

#pragma warning(disable:4200)
#pragma warning(disable:4201)
#pragma warning(disable:4213)
#pragma warning(disable:4214)
#pragma warning(disable:4701)
#pragma warning(disable:4053)
#pragma warning(disable:4152)

#pragma comment(lib, "I:\\repos\\osdev\\carbon_v2\\x64\\Release\\carbkrnl.lib") // these dont work?
#pragma comment(lib, "I:\\repos\\osdev\\carbon_v2\\x64\\Release\\kdcom.lib")
#pragma comment(lib, "I:\\repos\\osdev\\carbon_v2\\x64\\Release\\pci.lib")

#include <ntbase.h>
#include <ntstatus.h>
#include <bootldr.h>

#include <rtl.h>
#include <ke.h>
#include <mm.h>
#include <ob.h>
#include <io.h>
#include <hal.h>
#include <ps.h>
#include <fsrtl.h>
#include <cm.h>

#include "../../core/ntuser/usersup.h"
