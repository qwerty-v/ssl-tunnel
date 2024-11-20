#include <ssl-tunnel/lib/memory.h>
#include <ssl-tunnel/lib/err.h>
#include <ssl-tunnel/lib/fd.h>
#include <ssl-tunnel/lib/proto.h>
#include <ssl-tunnel/lib/deque.h>
#include <ssl-tunnel/lib/io.h>
#include <ssl-tunnel/server/signal.h>
#include <ssl-tunnel/server/config.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/epoll.h>

const err_t ERR_SERVER_IFCONFIG_FAILED = {
        .msg = "error executing ifconfig"
};

typedef struct {
    memory_set_t m;

    config_t cfg;

    int tun_fd;
    int server_fd;
    int poll_fd;

    deque_t recv_buf;
    deque_t send_buf;

    // fixme a single client address
    struct sockaddr_in client_addr;
} _server_t;

static err_t _server_ifconfig_device_up(const config_t *cfg) {
    char *fmt = "ifconfig %s 10.8.0.1 mtu %d netmask 255.255.255.0 up";

    char *result_cmd = malloc(snprintf(0, 0, fmt, cfg->device_name, cfg->device_mtu) + 1);
    sprintf(result_cmd, fmt, cfg->device_name, cfg->device_mtu);

    printf("%s\n", result_cmd);
    int status = system(result_cmd);
    free(result_cmd);

    if (status != 0) {
        return ERR_SERVER_IFCONFIG_FAILED;
    }

    return ENULL;
}

static void _server_handle_socket_read(_server_t *srv) {
    io_udp_read(srv->server_fd, &srv->recv_buf, (struct sockaddr *) &srv->client_addr,
            &(unsigned int) {sizeof(struct sockaddr_in)});

    io_tun_write(srv->tun_fd, &srv->recv_buf);

    if (srv->recv_buf->len > 0) {
        fprintf(stderr, "warn: device busy\n");
    }
}

static void _server_handle_tun_read(_server_t *srv) {
    io_tun_read(srv->tun_fd, &srv->send_buf);

    io_udp_write(srv->server_fd, &srv->send_buf, (const struct sockaddr *) &srv->client_addr,
                 sizeof(struct sockaddr_in));

    if (srv->send_buf->len > 0) {
        fprintf(stderr, "warn: socket busy\n");
    }
}

static void _server_init(_server_t *srv) {
    memset(srv, 0, sizeof(_server_t));

    memory_set_init(&srv->m);

    srv->tun_fd = -1;
    srv->server_fd = -1;
    srv->poll_fd = -1;

    deque_init(&srv->recv_buf, sizeof(io_wire_t), &srv->m.allocator);
    deque_init(&srv->send_buf, sizeof(io_wire_t), &srv->m.allocator);
}

static void _server_free(_server_t *srv) {
    if (srv->poll_fd >= 0) {
        close(srv->poll_fd);
    }
    if (srv->server_fd >= 0) {
        close(srv->server_fd);
    }
    if (srv->tun_fd >= 0) {
        close(srv->tun_fd);
    }

    memory_set_free(&srv->m);
}

err_t server_main(int argc, char *argv[]) {
    _server_t srv;
    _server_init(&srv);

    err_t err = config_read("/etc/ssl-tunnel/server.yaml", &srv.m, &srv.cfg);
    if (!ERROR_OK(err)) {
        goto cleanup;
    }

    signal_init();

    if (!ERROR_OK(err = fd_tun_open(srv.cfg.device_name, &srv.tun_fd))) {
        goto cleanup;
    }

    if (!ERROR_OK(err = fd_set_nonblock(srv.tun_fd))) {
        goto cleanup;
    }

    if (!ERROR_OK(err = _server_ifconfig_device_up(&srv.cfg))) {
        goto cleanup;
    }

    if (!ERROR_OK(err = fd_udp_server_open(srv.cfg.server_port, &srv.server_fd))) {
        goto cleanup;
    }

    if (!ERROR_OK(err = fd_set_nonblock(srv.server_fd))) {
        goto cleanup;
    }

    if (!ERROR_OK(err = fd_poll_create(&srv.poll_fd))) {
        goto cleanup;
    }

    if (!ERROR_OK(err = fd_poll_add(srv.poll_fd, srv.tun_fd, POLL_READ))) {
        goto cleanup;
    }

    if (!ERROR_OK(err = fd_poll_add(srv.poll_fd, srv.server_fd, POLL_READ))) {
        goto cleanup;
    }

    struct epoll_event evs[2];

    printf("server is listening on port %d\n", srv.cfg.server_port);
    fflush(stdout);

    while (!signal_read_flag()) {
        int fd_ready;
        err = fd_poll_wait(srv.poll_fd, evs, 2, 0, &fd_ready);
        if (!ERROR_OK(err)) {
            goto cleanup;
        }

        for (int i = 0; i < fd_ready; i++) {
            struct epoll_event ev = evs[i];
            int fd = ev.data.fd;

            if (fd == srv.server_fd) {
                _server_handle_socket_read(&srv);
                continue;
            }

            if (fd != srv.tun_fd) {
                panicf("unexpected fd received");
            }

            _server_handle_tun_read(&srv);
        }
    }
    printf("signal received, exiting...\n");

cleanup:
    _server_free(&srv);
    return err;
}