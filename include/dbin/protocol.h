#pragma once

#define DBIN_MAGIC       0xDB1
#define DBIN_VERSION     1

enum dbin_type {
    DBIN_TYPE_MSG  = 0,
    DBIN_TYPE_ACK  = 1,
    DBIN_TYPE_PING = 2,
    DBIN_TYPE_PONG = 3
};

enum dbin_route_type {
    DBIN_ROUTE_USER = 0,
    DBIN_ROUTE_ROOM = 1
};

enum dbin_flags {
    DBIN_FLAG_VALID = 1 << 0
};

#define DBIN_MAX_MSG_LEN 4095
