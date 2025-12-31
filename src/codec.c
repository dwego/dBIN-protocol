#include "dbin/codec.h"
#include "dbin/protocol.h"
#include "dbin/bitio.h"

static int in_range_u32(u32 v, u32 max_inclusive) { return v <= max_inclusive; }

usize dbin_header_bits_v1(void) {
    return 92u;
}

usize dbin_header_bytes_v1(void) {
    return (dbin_header_bits_v1() + 7u) / 8u;
}

int dbin_validate(const dbin_msg_t *m) {
    if (!m) return DBIN_ERR_PARAM;

    if (!in_range_u32((u32)m->magic, 0xFFFu)) return DBIN_ERR_RANGE;
    if (!in_range_u32((u32)m->version, 0xFu)) return DBIN_ERR_RANGE;
    if (!in_range_u32((u32)m->type, 0x7u)) return DBIN_ERR_RANGE;

    if ((m->reserved & 0x7u) != 0u) return DBIN_ERR_FMT;

    if (m->user_id >= (1u << 20)) return DBIN_ERR_RANGE;
    if (m->route   >= (1u << 20)) return DBIN_ERR_RANGE;

    if ((u32)m->msg_len > DBIN_MAX_MSG_LEN) return DBIN_ERR_RANGE;

    if (m->type == DBIN_TYPE_ACK || m->type == DBIN_TYPE_PING || m->type == DBIN_TYPE_PONG) {
        if (m->msg_len != 0) return DBIN_ERR_FMT;
    }
    if (m->msg_len > 0 && !m->msg) return DBIN_ERR_PARAM;
    if (m->version != DBIN_VERSION) return DBIN_ERR_VER;
    if ((u32)m->magic != DBIN_MAGIC) return DBIN_ERR_MAGIC;

    return DBIN_OK;
}

usize dbin_encoded_size(const dbin_msg_t *m) {
    if (!m) return 0;

    // Physical bytes used:
    // - header bits (92) rounded up to bytes => 12 bytes
    // - then payload as full bytes
    return dbin_header_bytes_v1() + (usize)m->msg_len;
}

int dbin_encode(const dbin_msg_t *m, u8 *out, usize cap, usize *out_len) {
    if (!m || !out || !out_len) return DBIN_ERR_PARAM;

    int vr = dbin_validate(m);
    if (vr != DBIN_OK) return vr;

    usize need = dbin_encoded_size(m);
    if (cap < need) return DBIN_ERR_BUF;

    // IMPORTANT: because bitio_write_bits can clear bits when writing 0,
    // it's safe even if buffer isn't zeroed, but it's still a good habit
    // to zero at least the region you'll touch for deterministic output.
    for (usize i = 0; i < need; i++) out[i] = 0;

    bitio_t b;
    bitio_init(&b, out, cap);

    if (bitio_write_bits(&b, (u32)m->magic,   12)) return DBIN_ERR_BUF;
    if (bitio_write_bits(&b, (u32)m->version,  4)) return DBIN_ERR_BUF;
    if (bitio_write_bits(&b, (u32)m->type,     3)) return DBIN_ERR_BUF;
    if (bitio_write_bits(&b, (u32)(m->valid ? 1u : 0u), 1)) return DBIN_ERR_BUF;
    if (bitio_write_bits(&b, (u32)(m->is_room ? 1u : 0u), 1)) return DBIN_ERR_BUF;
    if (bitio_write_bits(&b, (u32)(m->reserved & 0x7u), 3)) return DBIN_ERR_BUF;

    if (bitio_write_bits(&b, (u32)m->user_id, 20)) return DBIN_ERR_BUF;
    if (bitio_write_bits(&b, (u32)m->route,   20)) return DBIN_ERR_BUF;
    if (bitio_write_bits(&b, (u32)m->msg_id,  16)) return DBIN_ERR_BUF;
    if (bitio_write_bits(&b, (u32)m->msg_len, 12)) return DBIN_ERR_BUF;

    if (bitio_align_byte(&b)) return DBIN_ERR_BUF;

    if (m->msg_len > 0) {
        if (bitio_write_bytes(&b, m->msg, (usize)m->msg_len)) return DBIN_ERR_BUF;
    }

    *out_len = bitio_bytes_used(&b);
    return DBIN_OK;
}

int dbin_decode(const u8 *in, usize in_len, dbin_msg_t *out) {
    if (!in || !out) return DBIN_ERR_PARAM;

    if (in_len < dbin_header_bytes_v1()) return DBIN_ERR_BUF;

    out->magic = 0;
    out->version = 0;
    out->type = 0;
    out->valid = 0;
    out->is_room = 0;
    out->reserved = 0;
    out->user_id = 0;
    out->route = 0;
    out->msg_id = 0;
    out->msg_len = 0;
    out->msg = 0;

    bitio_t r;
    bitio_init(&r, (u8*)in, in_len);

    u32 tmp = 0;

    if (bitio_read_bits(&r, 12, &tmp)) return DBIN_ERR_BUF;
    out->magic = (u16)tmp;

    if ((u32)out->magic != DBIN_MAGIC) return DBIN_ERR_MAGIC;

    if (bitio_read_bits(&r, 4, &tmp)) return DBIN_ERR_BUF;
    out->version = (u8)tmp;

    if (out->version != DBIN_VERSION) return DBIN_ERR_VER;

    if (bitio_read_bits(&r, 3, &tmp)) return DBIN_ERR_BUF;
    out->type = (u8)tmp;

    if (bitio_read_bits(&r, 1, &tmp)) return DBIN_ERR_BUF;
    out->valid = (tmp != 0);

    if (bitio_read_bits(&r, 1, &tmp)) return DBIN_ERR_BUF;
    out->is_room = (tmp != 0);

    if (bitio_read_bits(&r, 3, &tmp)) return DBIN_ERR_BUF;
    out->reserved = (u8)tmp;
    if ((out->reserved & 0x7u) != 0u) return DBIN_ERR_FMT;

    if (bitio_read_bits(&r, 20, &tmp)) return DBIN_ERR_BUF;
    out->user_id = tmp;

    if (bitio_read_bits(&r, 20, &tmp)) return DBIN_ERR_BUF;
    out->route = tmp;

    if (bitio_read_bits(&r, 16, &tmp)) return DBIN_ERR_BUF;
    out->msg_id = (u16)tmp;

    if (bitio_read_bits(&r, 12, &tmp)) return DBIN_ERR_BUF;
    out->msg_len = (u16)tmp;

    if ((u32)out->msg_len > DBIN_MAX_MSG_LEN) return DBIN_ERR_RANGE;

    if (bitio_align_byte(&r)) return DBIN_ERR_BUF;

    usize payload_off = r.bitpos / 8;
    if (payload_off > in_len) return DBIN_ERR_BUF;

    usize remaining = in_len - payload_off;
    if ((usize)out->msg_len > remaining) return DBIN_ERR_BUF;

    if (out->type == DBIN_TYPE_ACK || out->type == DBIN_TYPE_PING || out->type == DBIN_TYPE_PONG) {
        if (out->msg_len != 0) return DBIN_ERR_FMT;
    }

    out->msg = (out->msg_len > 0) ? (const u8*)(in + payload_off) : 0;

    return DBIN_OK;
}
