#pragma once

#include "dbin_types.h"

typedef struct {
    u8    *buf;       // bytes buffer pointer
    usize  cap_bytes; // buffer capacity
    usize  bitpos;    // current position in BITS
} bitio_t;

void bitio_init(bitio_t *b, u8 *buf, usize cap_bytes);
int bitio_write_bits(bitio_t *b, u32 value, int nbits);
int bitio_read_bits(bitio_t *b, int nbits, u32 *out);
int bitio_align_byte(bitio_t *b);
int bitio_write_bytes(bitio_t *b, const u8 *src, usize n);
int bitio_read_bytes(bitio_t *b, u8 *dst, usize n);
usize bitio_bytes_used(const bitio_t *b);