


#pragma once

typedef union _SEP_TOKEN_PRIVILEGES {
    struct {
        ULONG64 SeCreateTokenPrivilege : 1;
    };
    ULONG64 Enabled;
} SEP_TOKEN_PRIVILEGES, *PSEP_TOKEN_PRIVILEGES;

typedef struct _TOKEN {

    SEP_TOKEN_PRIVILEGES Privileges;
} TOKEN, *PTOKEN;

#define ACE_TYPE_ACCESS_ALLOWED 0x01

typedef struct _ACE_HEADER {
    UCHAR  Type;
    USHORT Size;

} ACE_HEADER, *PACE_HEADER;

typedef struct _ACE_ACCESS_ALLOWED {
    UCHAR unk;
} ACE_ACCESS_ALLOWED, *PACE_ACCESS_ALLOWED;

typedef struct _ACL {
    ULONG AceCount;


} ACL, *PACL;
