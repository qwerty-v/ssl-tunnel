#include <ssl-tunnel/signal.h>
#include <ssl-tunnel/memory.h>
#include <ssl-tunnel/errors.h>
#include <ssl-tunnel/flag.h>
#include <ssl-tunnel/fd.h>
#include <ssl-tunnel/config.h>
#include <ssl-tunnel/proto.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <errno.h>

typedef struct {
    config_t cfg;

    int tun_fd;
    int server_fd;
    int poll_fd;

    alloc_t allocator;
    slice_t recv_buf;
    slice_t send_buf;

    // a single client address
    struct sockaddr_in client_addr;

    volatile bool sig_received;
} server_t;

const err_t ERROR_IFCONFIG_FAILED = {
        .ok = false,
        .msg = "error executing ifconfig"
};

err_t _ifconfig_device_up(const config_t *cfg) {
    char *fmt = "ifconfig %s 10.8.0.1 mtu %d netmask 255.255.255.0 up";

    char *result_cmd = malloc(snprintf(0, 0, fmt, cfg->device_name, cfg->device_mtu) + 1);
    sprintf(result_cmd, fmt, cfg->device_name, cfg->device_mtu);

    printf("%s\n", result_cmd);
    int status = system(result_cmd);
    free(result_cmd);

    if (status != 0) {
        return ERROR_IFCONFIG_FAILED;
    }

    return ERROR_OK;
}

typedef struct {
    int len;
    proto_wire_t packet;
} wire_data_t;

void _handle_socket_read(server_t *srv) {
    // udp -> queue
    for (wire_data_t d; 1; ) {
        d.len = recvfrom(srv->server_fd, d.packet.data, MAX_MTU, 0, (struct sockaddr *) &srv->client_addr,
                &(unsigned int) {sizeof(struct sockaddr_in)});
        if (d.len < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            panicf("error calling recvfrom (errno %d)", errno);
        }

        err_t err;
        if (!ERR_OK(err = slice_append(&srv->recv_buf, &d))) {
            panicf("error calling slice_append on recv_buf (err.msg %s)", err.msg);
        }
    }

    // queue -> tun
    while (srv->recv_buf.len > 0) {
        wire_data_t *d = srv->recv_buf.array;

        int n = write(srv->tun_fd, d->packet.data, d->len);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            panicf("error calling write (errno %d)", errno);
        }
        if (n != d->len) {
            panicf("could not write whole buffer into device");
        }

        err_t err;
        if (!ERR_OK(err = slice_remove(&srv->recv_buf, 0))) {
            panicf("error calling slice_remove on recv_buf (err.msg %s)", err.msg);
        }
    }

    if (srv->recv_buf.len > 0) {
        fprintf(stderr, "warn: device busy\n");
    }
}

void _handle_tun_read(server_t *srv) {
    // tun -> queue
    for (wire_data_t d; 1; ) {
        d.len = read(srv->tun_fd, d.packet.data, MAX_MTU);
        if (d.len < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            panicf("error calling read (errno %d)", errno);
        }

        err_t err;
        if (!ERR_OK(err = slice_append(&srv->send_buf, &d))) {
            panicf("error calling slice_append on send_buf (err.msg %s)", err.msg);
        }
    }

    // queue -> udp
    while (srv->send_buf.len > 0) {
        wire_data_t *d = srv->send_buf.array;

        int n = sendto(srv->server_fd, d->packet.data, d->len, 0,
                       (const struct sockaddr *) &srv->client_addr, sizeof(struct sockaddr_in));
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            panicf("error calling sendto (errno %d)", errno);
        }
        if (n != d->len) {
            panicf("could not send whole buffer to client");
        }

        err_t err;
        if (!ERR_OK(err = slice_remove(&srv->send_buf, 0))) {
            panicf("error calling slice_remove on send_buf (err.msg %s)", err.msg);
        }
    }

    if (srv->send_buf.len > 0) {
        fprintf(stderr, "warn: socket busy\n");
    }
}

err_t server_main(int argc, char *argv[]) {
    alloc_pool_t p;
    alloc_pool_init(&p);

    err_t err;
    char *cfg_path;
    if (!ERR_OK(err = flag_parse(argc, argv, &cfg_path))) {
        goto cleanup_1;
    }

    server_t srv;
    memset(&srv, 0, sizeof(server_t));

    if (!ERR_OK(err = config_read(cfg_path, &srv.cfg))) {
        goto cleanup_1;
    }

    if (!ERR_OK(err = alloc_pool_get_allocator(&p, &srv.allocator))) {
        goto cleanup_1;
    }

    slice_init(&srv.recv_buf, sizeof(wire_data_t), (optional_alloc_t) {
        .present = true,
        .value = srv.allocator
    });

    slice_init(&srv.send_buf, sizeof(wire_data_t), (optional_alloc_t) {
        .present = true,
        .value = srv.allocator
    });

    signal_init(&srv.sig_received);

    if (!ERR_OK(err = fd_tun_open(srv.cfg.device_name, &srv.tun_fd))) {
        goto cleanup_1;
    }

    if (!ERR_OK(err = fd_set_nonblock(srv.tun_fd))) {
        goto cleanup_2;
    }

    if (!ERR_OK(err = _ifconfig_device_up(&srv.cfg))) {
        goto cleanup_2;
    }

    if (!ERR_OK(err = fd_udp_server_open(srv.cfg.server_port, &srv.server_fd))) {
        goto cleanup_2;
    }

    if (!ERR_OK(err = fd_set_nonblock(srv.server_fd))) {
        goto cleanup_3;
    }

    if (!ERR_OK(err = fd_poll_create(&srv.poll_fd))) {
        goto cleanup_3;
    }

    if (!ERR_OK(err = fd_poll_add(srv.poll_fd, srv.tun_fd, POLL_READ))) {
        goto cleanup_4;
    }

    if (!ERR_OK(err = fd_poll_add(srv.poll_fd, srv.server_fd, POLL_READ))) {
        goto cleanup_4;
    }

    struct epoll_event evs[2];

    printf("server is listening on port %d\n", srv.cfg.server_port);
    fflush(stdout);

    while (!srv.sig_received) {
        int fd_ready;
        if (!ERR_OK(err = fd_poll_wait(srv.poll_fd, evs, 2, 0, &fd_ready))) {
            goto cleanup_4;
        }

        for (int i = 0; i < fd_ready; i++) {
            struct epoll_event ev = evs[i];
            int fd = ev.data.fd;

            if (fd == srv.server_fd) {
                _handle_socket_read(&srv);
                continue;
            }

            if (fd != srv.tun_fd) {
                panicf("unexpected fd received");
            }

            _handle_tun_read(&srv);
        }
    }
    printf("signal received, terminating server...\n");

cleanup_4:
    close(srv.poll_fd);
cleanup_3:
    close(srv.server_fd);
cleanup_2:
    close(srv.tun_fd);
cleanup_1:
    alloc_pool_free(&p);
    return err;
}