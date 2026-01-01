// examples/server.c
// Build:
//   gcc -O2 -Wall -Wextra -Iinclude examples/00_socket/server.c src/bitio.c src/codec.c -o server
// Run:
//   ./server 127.0.0.1 9000

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "dbin/types.h"
#include "dbin/protocol.h"
#include "dbin/dbin.h"
#include "dbin/codec.h"

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
        if (n == 0) return 1; // connection closed
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

static int make_listener(const char *ip, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    addr.sin_addr.s_addr = inet_addr(ip);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    if (listen(fd, 16) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s <ip> <port>\n", argv[0]);
        return 1;
    }
    const char *ip = argv[1];
    int port = atoi(argv[2]);

    int lf = make_listener(ip, port);
    if (lf < 0) {
        perror("listen");
        return 1;
    }
    printf("[server] listening on %s:%d\n", ip, port);

    struct sockaddr_in cli;
    socklen_t cli_len = sizeof(cli);
    int cfd = accept(lf, (struct sockaddr*)&cli, &cli_len);
    if (cfd < 0) {
        perror("accept");
        return 1;
    }
    printf("[server] client connected\n");

    u8 inbuf[8192];
    u8 outbuf[256]; // ACK is tiny

    for (;;) {
        usize in_len = 0;
        if (read_frame(cfd, inbuf, (usize)sizeof(inbuf), &in_len)) {
            printf("[server] connection closed / frame read error\n");
            break;
        }

        dbin_msg_t msg;
        int rc = dbin_decode(inbuf, in_len, &msg);
        if (rc != DBIN_OK) {
            printf("[server] decode error: %d\n", rc);
            continue;
        }

        if (msg.type == DBIN_TYPE_MSG) {
            dbin_msg_t ack;
            ack.magic = (u16)DBIN_MAGIC;
            ack.version = (u8)DBIN_VERSION;
            ack.type = (u8)DBIN_TYPE_ACK;
            ack.valid = 1;
            ack.is_room = msg.is_room;
            ack.reserved = 0;

            ack.user_id = msg.user_id;
            ack.route   = msg.route;
            ack.msg_id  = msg.msg_id;
            ack.msg_len = 0;
            ack.msg = 0;

            usize out_len = 0;
            rc = dbin_encode(&ack, outbuf, (usize)sizeof(outbuf), &out_len);
            if (rc != DBIN_OK) {
                printf("[server] encode ack error: %d\n", rc);
                continue;
            }
            if (write_frame(cfd, outbuf, out_len)) {
                printf("[server] send error\n");
                break;
            }
        }
    }

    close(cfd);
    close(lf);
    return 0;
}
