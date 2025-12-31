#include "dbin/types.h"
#include "dbin/protocol.h"
#include "dbin/dbin.h"
#include "dbin/codec.h"

extern int printf(const char*, ...);

static void dump_hex(const u8 *buf, usize n) {
    for (usize i = 0; i < n; i++) {
        printf("%02X ", (unsigned)buf[i]);
    }
    printf("\n");
}

static const char* err_str(int rc) {
    switch (rc) {
        case DBIN_OK:        return "DBIN_OK";
        case DBIN_ERR_PARAM: return "DBIN_ERR_PARAM";
        case DBIN_ERR_RANGE: return "DBIN_ERR_RANGE";
        case DBIN_ERR_BUF:   return "DBIN_ERR_BUF";
        case DBIN_ERR_MAGIC: return "DBIN_ERR_MAGIC";
        case DBIN_ERR_VER:   return "DBIN_ERR_VER";
        case DBIN_ERR_FMT:   return "DBIN_ERR_FMT";
        default:             return "DBIN_ERR_UNKNOWN";
    }
}

int main(void) {
    const u8 msg_bytes[] = { 'o', 'i' };

    dbin_msg_t m;
    m.magic    = (u16)DBIN_MAGIC;     // 12-bit value stored in u16
    m.version  = (u8)DBIN_VERSION;    // 4-bit on wire
    m.type     = (u8)DBIN_TYPE_MSG;   // 3-bit on wire
    m.valid    = 1;
    m.is_room  = 1;
    m.reserved = 0;

    m.user_id  = 5321;                // < 2^20
    m.route    = 77;                  // room_id (since is_room=1)
    m.msg_id   = 42;
    m.msg_len  = (u16)2;
    m.msg      = msg_bytes;

    // ---- encode ----
    u8 out[128];
    usize out_len = 0;

    int rc = dbin_encode(&m, out, (usize)sizeof(out), &out_len);
    if (rc != DBIN_OK) {
        printf("dbin_encode failed: %s (%d)\n", err_str(rc), rc);
        return 1;
    }

    printf("Encoded %lu bytes:\n", (unsigned long)out_len);
    dump_hex(out, out_len);

    // ---- decode ----
    dbin_msg_t d;
    rc = dbin_decode(out, out_len, &d);
    if (rc != DBIN_OK) {
        printf("dbin_decode failed: %s (%d)\n", err_str(rc), rc);
        return 2;
    }

    printf("\nDecoded:\n");
    printf(" magic   = 0x%03X\n", (unsigned)d.magic);
    printf(" version = %u\n", (unsigned)d.version);
    printf(" type    = %u\n", (unsigned)d.type);
    printf(" valid   = %u\n", (unsigned)d.valid);
    printf(" is_room = %u\n", (unsigned)d.is_room);
    printf(" reserved= %u\n", (unsigned)d.reserved);
    printf(" user_id = %u\n", (unsigned)d.user_id);
    printf(" route   = %u\n", (unsigned)d.route);
    printf(" msg_id  = %u\n", (unsigned)d.msg_id);
    printf(" msg_len = %u\n", (unsigned)d.msg_len);

    if (d.msg_len > 0 && d.msg) {
        printf(" msg     = \"");
        for (u16 i = 0; i < d.msg_len; i++) {
            printf("%c", (char)d.msg[i]);
        }
        printf("\"\n");
    } else {
        printf(" msg     = <none>\n");
    }

    int ok = 1;
    ok &= (d.magic == (u16)DBIN_MAGIC);
    ok &= (d.version == (u8)DBIN_VERSION);
    ok &= (d.type == (u8)DBIN_TYPE_MSG);
    ok &= (d.valid == 1);
    ok &= (d.is_room == 1);
    ok &= (d.reserved == 0);
    ok &= (d.user_id == 5321);
    ok &= (d.route == 77);
    ok &= (d.msg_id == 42);
    ok &= (d.msg_len == 2);
    ok &= (d.msg && d.msg[0] == 'o' && d.msg[1] == 'i');

    printf("\nResult: %s\n", ok ? "OK (roundtrip matched)" : "FAIL (mismatch)");
    return ok ? 0 : 3;
}
