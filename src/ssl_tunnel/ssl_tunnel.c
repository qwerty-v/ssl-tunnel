#include "socket.h"
#include "poll.h"
#include "tun.h"
#include "proto.h"
#include "ifconfig.h"

#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <libc.h>
#include <sys/errno.h>
#include <sys/uio.h>

struct sockaddr_in client_addr; // fixme remember sessions

int ssl_tunnel_read_client(struct buffer *b, int fd) {
    assert(b->size == 0); // fixme add buffer offset

    socklen_t addr_len = sizeof(client_addr);
    b->size = (int) recvfrom(fd, b->buf, b->capacity, 0, (struct sockaddr *) &client_addr, &addr_len);

//    const proto_iphdr *ip = (const proto_iphdr *) ev_ctx->recv_buf.b;
//    hash_add(ev_ctx->hash, ip->ip_src, client_addr);

    return b->size;
}

int ssl_tunnel_read_tun(struct buffer *b, int fd) {
    assert(b->size == 0); // fixme add buffer offset

    b->size = (int) read(fd, b->buf, b->capacity);
    return b->size;
}

int ssl_tunnel_write_client(struct buffer *b, int fd) {
//    const proto_iphdr *ip = (const proto_iphdr *) ev_ctx->send_buf.b;
//    struct sockaddr_in client_addr = *(struct sockaddr_in *) hash_get(ev_ctx->hash, ip->ip_src);

    socklen_t addr_len = sizeof(client_addr);
    int n = (int) sendto(fd, &b->buf[sizeof(uint32_t)], b->size - sizeof(uint32_t),
                         0, (struct sockaddr *) &client_addr, addr_len);
    assert(n == b->size - sizeof(uint32_t)); // fixme add buffer offset

    b->size = 0;
    return n;
}

int ssl_tunnel_write_tun(struct buffer *b, int fd) {
    assert(b->size > 0);

    struct iovec iv[2];

    uint32_t header;
    if (proto_ip_get_version(*b) == 4) {
        header = htonl(AF_INET);
    } else {
        header = htonl(AF_INET6);
    }
    iv[0].iov_base = &header;
    iv[0].iov_len = sizeof(header);

    iv[1].iov_base = b->buf;
    iv[1].iov_len = b->size;

    int n = (int) writev(fd, iv, 2);
    assert(n - sizeof(header) == b->size); // fixme add buffer offset

    b->size = 0;
    return n;
}

void free_ev_ctx(struct ev_loop_context *ev_ctx) {
    free_buf(ev_ctx->send_buf);
    free_buf(ev_ctx->recv_buf);
    free(ev_ctx);
}

void free_ctx(struct context *ctx) {
    close(ctx->server_sd);
    close(ctx->tun_fd);
    close(ctx->poll_fd);
    free(ctx);
}

void cleanup(struct context *ctx, struct ev_loop_context *ev_ctx) {
    if (ev_ctx) {
        free_ev_ctx(ev_ctx);
    }
    if (ctx) {
        free_ctx(ctx);
    }
}

int ssl_tunnel_main(int argc, char *argv[]) {
    struct context *ctx = malloc(sizeof(struct context));
    memset(ctx, 0, sizeof(struct context));

    ctx->port = 1026; // fixme add config file

    ctx->tun_fd = tun_open();
    if (ctx->tun_fd < 0) {
        cleanup(ctx, NULL);
        return -1;
    }

    if (ifconfig_device_up() < 0) {
        cleanup(ctx, NULL);
        return -1;
    }

    ctx->server_sd = socket_create_server(ctx);
    if (ctx->server_sd < 0) {
        cleanup(ctx, NULL);
        return -1;
    }

    ctx->poll_fd = poll_create();
    if (ctx->poll_fd < 0) {
        cleanup(ctx, NULL);
        return -1;
    }

    if (poll_add(ctx->poll_fd, ctx->tun_fd, POLL_READ) < 0) {
        cleanup(ctx, NULL);
        return -1;
    }

    if (poll_add(ctx->poll_fd, ctx->server_sd, POLL_READ) < 0) {
        cleanup(ctx, NULL);
        return -1;
    }

    struct ev_loop_context *ev_ctx = malloc(sizeof(struct ev_loop_context));
    memset(ev_ctx, 0, sizeof(struct ev_loop_context));

    const size_t buf_size = 8 * 1024;
    ev_ctx->send_buf = alloc_buf(buf_size);
    ev_ctx->recv_buf = alloc_buf(buf_size);

    ctx->running = true;
    printf("io wait\n");
    fflush(stdout);

    const int nevents = 1000;
    struct kevent events[nevents];
    while (ctx->running) {
        int nev = kevent(ctx->poll_fd, NULL, 0, events, nevents, NULL);
        if (nev < 0) {
            cleanup(ctx, ev_ctx);
            return -1;
        }

        for (int i = 0; i < nev; i++) {
            int fd = (int) events[i].ident;

            if (fd == ctx->server_sd) {
                if (ssl_tunnel_read_client(&ev_ctx->recv_buf, ctx->server_sd) < 0) {
                    cleanup(ctx, ev_ctx);
                    return -1;
                }
                if (ssl_tunnel_write_tun(&ev_ctx->recv_buf, ctx->tun_fd) < 0) {
                    cleanup(ctx, ev_ctx);
                    return -1;
                }
                continue;
            }

            assert(fd == ctx->tun_fd);
            if (ssl_tunnel_read_tun(&ev_ctx->send_buf, ctx->tun_fd) < 0) {
                cleanup(ctx, ev_ctx);
                return -1;
            }
            if (ssl_tunnel_write_client(&ev_ctx->send_buf, ctx->server_sd) < 0) {
                cleanup(ctx, ev_ctx);
                return -1;
            }
        }
    }

    cleanup(ctx, ev_ctx);
    return 0;
}

int main(int argc, char *argv[]) {
    int r = ssl_tunnel_main(argc, argv);
    if (r < 0) {
        char *str = strerror(errno);
        printf("%s", str);
    }
    return r;
}