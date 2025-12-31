#pragma once

#include "dbin_types.h"

typedef struct dbin_msg {
    u16 magic;      // uses only 12 bits (0..0xFFF)
    u8  version;    // uses only 4 bits  (0..15)
    u8  type;       // uses only 3 bits  (0..7)
    _Bool valid;    // 1 bit on wire
    _Bool is_room;  // 1 bit on wire

    u8  reserved;   // uses only 3 bits (must be 0 for v1)

    u32 user_id;    // uses only 20 bits (0..(2^20-1))
    u32 route;      // uses only 20 bits (room_id or to_user_id)

    u16 msg_id;     // 16 bits
    u16 msg_len;    // uses only 12 bits (0..4095)
    const u8 *msg;  // msg_len bytes (UTF-8)
} dbin_msg_t;