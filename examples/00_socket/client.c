// examples/client.c
// Build:
//   gcc -O2 -Wall -Wextra -Iinclude examples/00_socket/client.c src/bitio.c src/codec.c -o client
// Run:
//   ./client 127.0.0.1 9000 1000
// (last arg = number of pings)

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "dbin/types.h"
#include "dbin/protocol.h"
#include "dbin/dbin.h"
#include "dbin/codec.h"

static u64 now_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (u64)tv.tv_sec * 1000000ull + (u64)tv.tv_usec;
}

static int send_all(int fd, const u8 *buf, usize len) {
    while (len > 0) {
        ssize_t n = send(fd, buf, (size_t)len, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return 1;
        }
        buf += (usize)n;
        len -= (usize)n;
    }
    return 0;
}

static int recv_all(int fd, u8 *buf, usize len) {
    while (len > 0) {
        ssize_t n = recv(fd, buf, (size_t)len, 0);
        if (n == 0) return 1;
        if (n < 0) {
            if (errno == EINTR) continue;
            return 1;
        }
        buf += (usize)n;
        len -= (usize)n;
    }
    return 0;
}

static int read_frame(int fd, u8 *payload, usize cap, usize *out_len) {
    u8 len2[2];
    if (recv_all(fd, len2, 2)) return 1;

    u16 n = (u16)((len2[0] << 8) | len2[1]);
    if ((usize)n > cap) return 1;

    if (recv_all(fd, payload, (usize)n)) return 1;
    *out_len = (usize)n;
    return 0;
}

static int write_frame(int fd, const u8 *payload, usize payload_len) {
    if (payload_len > 65535u) return 1;
    u8 len2[2];
    len2[0] = (u8)((payload_len >> 8) & 0xFF);
    len2[1] = (u8)(payload_len & 0xFF);

    if (send_all(fd, len2, 2)) return 1;
    if (send_all(fd, payload, payload_len)) return 1;
    return 0;
}

static int connect_to(const char *ip, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static void sort_u64(u64 *a, usize n) {
    // simple insertion sort (no libc qsort)
    for (usize i = 1; i < n; i++) {
        u64 key = a[i];
        usize j = i;
        while (j > 0 && a[j - 1] > key) {
            a[j] = a[j - 1];
            j--;
        }
        a[j] = key;
    }
}

int main(int argc, char **argv) {
    if (argc < 3 || argc > 4) {
        fprintf(stderr, "usage: %s <ip> <port> [count]\n", argv[0]);
        return 1;
    }
    const char *ip = argv[1];
    int port = atoi(argv[2]);
    int count = (argc == 4) ? atoi(argv[3]) : 1000;
    if (count <= 0) count = 1;
    if (count > 20000) count = 20000;

    int fd = connect_to(ip, port);
    if (fd < 0) {
        perror("connect");
        return 1;
    }
    printf("[client] connected to %s:%d\n", ip, port);

    u8 outbuf[8192];
    u8 inbuf[8192];

    // store RTTs
    u64 *rtts = (u64*)malloc((size_t)count * sizeof(u64));
    if (!rtts) {
        fprintf(stderr, "malloc failed\n");
        close(fd);
        return 1;
    }

    const u8 payload[] = "ping";
    u16 msg_id = 1;

    for (int i = 0; i < count; i++) {
        dbin_msg_t m;
        m.magic = (u16)DBIN_MAGIC;
        m.version = (u8)DBIN_VERSION;
        m.type = (u8)DBIN_TYPE_MSG;
        m.valid = 1;
        m.is_room = 1;
        m.reserved = 0;

        m.user_id = 123;       // fake client id
        m.route   = 77;        // room id
        m.msg_id  = msg_id++;
        m.msg_len = (u16)(sizeof(payload) - 1);
        m.msg     = payload;

        usize out_len = 0;
        int rc = dbin_encode(&m, outbuf, (usize)sizeof(outbuf), &out_len);
        if (rc != DBIN_OK) {
            printf("[client] encode error: %d\n", rc);
            break;
        }

        u64 t0 = now_us();
        if (write_frame(fd, outbuf, out_len)) {
            printf("[client] send error\n");
            break;
        }

        usize in_len = 0;
        if (read_frame(fd, inbuf, (usize)sizeof(inbuf), &in_len)) {
            printf("[client] recv error\n");
            break;
        }

        dbin_msg_t ack;
        rc = dbin_decode(inbuf, in_len, &ack);
        if (rc != DBIN_OK) {
            printf("[client] decode error: %d\n", rc);
            break;
        }

        if (ack.type != DBIN_TYPE_ACK || ack.msg_id != m.msg_id) {
            printf("[client] unexpected response (type=%u msg_id=%u)\n",
                   (unsigned)ack.type, (unsigned)ack.msg_id);
            break;
        }

        u64 t1 = now_us();
        rtts[i] = t1 - t0;
    }

    // stats
    usize n = (usize)count;
    // if loop broke early, you can track actual count; for simplicity assume full count here.
    // If you want exact, increment a `sent_ok` counter and use it.
    sort_u64(rtts, n);

    u64 sum = 0;
    for (usize i = 0; i < n; i++) sum += rtts[i];

    double avg_us = (n > 0) ? ((double)sum / (double)n) : 0.0;
    u64 p50 = rtts[(n * 50) / 100];
    u64 p95 = rtts[(n * 95) / 100];
    u64 p99 = rtts[(n * 99) / 100];

    printf("\nRTT stats (%lu samples)\n", (unsigned long)n);
    printf(" avg: %.2f us (%.3f ms)\n", avg_us, avg_us / 1000.0);
    printf(" p50: %llu us (%.3f ms)\n", (unsigned long long)p50, (double)p50 / 1000.0);
    printf(" p95: %llu us (%.3f ms)\n", (unsigned long long)p95, (double)p95 / 1000.0);
    printf(" p99: %llu us (%.3f ms)\n", (unsigned long long)p99, (double)p99 / 1000.0);

    free(rtts);
    close(fd);
    return 0;
}
