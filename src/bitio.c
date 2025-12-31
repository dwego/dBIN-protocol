#include "dbin/bitio.h"


static int valid_bit_pos(bitio_t *b, int nbits) {
    if (!b || !b->buf || nbits < 0) return 0;
    if (nbits > 32) return 0;

    usize total_bits = b->cap_bytes * 8;
    return b->bitpos + (usize)nbits <= total_bits;
}


void bitio_init(bitio_t *b, u8 *buf, usize cap_bytes) {
    b->buf = buf;
    b->cap_bytes = cap_bytes;
    b->bitpos = 0;
}

int bitio_write_bits(bitio_t *b, u32 value, int nbits) {
    if (!valid_bit_pos(b, nbits)) return 1;

    for (int i = 0; i < nbits; i++) {
        u8 bit = (value >> (nbits - 1 - i)) & 1u;

        usize byte = b->bitpos / 8;
        u8 bit_in_byte = (u8)(7 - (b->bitpos % 8));

        if (bit)
            b->buf[byte] |=  (u8)(1u << bit_in_byte);
        else
            b->buf[byte] &= (u8)~(1u << bit_in_byte);

        b->bitpos++;
    }
    return 0;
}

int bitio_read_bits(bitio_t *b, int nbits, u32 *out_value) {
    if (!out_value) return 1;
    *out_value = 0;

    if (!valid_bit_pos(b, nbits)) return 1;

    for (int i = 0; i < nbits; i++) {
        usize byte = b->bitpos / 8;
        u8 bit_in_byte = (u8)(7 - (b->bitpos % 8));
        u8 bit = (b->buf[byte] >> bit_in_byte) & 1u;

        *out_value = (*out_value << 1) | (u32)bit;
        b->bitpos++;
    }
    return 0;
}

int bitio_align_byte(bitio_t *b) {
    if (!b || !b->buf) return 1;

    usize rem = b->bitpos % 8;
    if (rem == 0) return 0;

    usize add = 8 - rem;
    usize total_bits = b->cap_bytes * 8;
    if (b->bitpos + add > total_bits) return 1;

    b->bitpos += add;
    return 0;
}

int bitio_write_bytes(bitio_t *b, const u8 *src, usize n) {
    if (!b || !b->buf || !src) return 1;
    if ((b->bitpos % 8) != 0) return 1;

    usize byte_pos = b->bitpos / 8;
    if (byte_pos + n > b->cap_bytes) return 1;

    for (usize i = 0; i < n; i++) {
        b->buf[byte_pos + i] = src[i];
    }

    b->bitpos += n * 8;
    return 0;
}

int bitio_read_bytes(bitio_t *b, u8 *dst, usize n) {
    if (!b || !b->buf || !dst) return 1;
    if ((b->bitpos % 8) != 0) return 1;

    usize byte_pos = b->bitpos / 8;
    if (byte_pos + n > b->cap_bytes) return 1;

    for (usize i = 0; i < n; i++) {
        dst[i] = b->buf[byte_pos + i];
    }

    b->bitpos += n * 8;
    return 0;
}

usize bitio_bytes_used(const bitio_t *b) {
    if (!b) return 0;
    return (b->bitpos + 7) / 8;
}
