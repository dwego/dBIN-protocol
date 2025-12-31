#pragma once
#include "dbin/types.h"
#include "dbin/dbin.h"

enum dbin_err {
    DBIN_OK = 0,
    DBIN_ERR_PARAM = 1,
    DBIN_ERR_RANGE = 2,
    DBIN_ERR_BUF   = 3,
    DBIN_ERR_MAGIC = 4,
    DBIN_ERR_VER   = 5,
    DBIN_ERR_FMT   = 6
};

int   dbin_validate(const dbin_msg_t *m);

usize dbin_header_bits_v1(void);
usize dbin_header_bytes_v1(void);

// Returns bytes required for encoding (header rounded up + msg_len; includes any physical rounding).
usize dbin_encoded_size(const dbin_msg_t *m);

// Encode message into `out`. `out_len` returns total bytes written.
int   dbin_encode(const dbin_msg_t *m, u8 *out, usize cap, usize *out_len);

// Decode from `in`. `out->msg` will point inside `in` (zero-copy) when applicable.
int   dbin_decode(const u8 *in, usize in_len, dbin_msg_t *out);
