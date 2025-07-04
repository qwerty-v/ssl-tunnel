#include <ssl-tunnel/backend/config.h>
#include <ssl-tunnel/backend/tunnel.h>

#include <ssl-tunnel/lib/memscope.h>
#include <ssl-tunnel/lib/fd.h>

#include <stdlib.h> // getenv
#include <stdio.h> // printf
#include <errno.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h> // close
#include <stdint.h> // uint64_t

static int signal_fd = -1;

static void signal_handler(int unused) {
    (void) unused;

    (void) fd_eventfd_write(signal_fd);
}

void signal_init() {
    signal(SIGHUP, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGTERM, signal_handler);
}

typedef struct {
    memscope_t m;

    config_t cfg;

    int tun_fd;
    int socket_fd;
} main_t;

void main_init(main_t *m) {
    memset(m, 0, sizeof(main_t));

    memscope_init(&m->m);

    config_init(&m->cfg, &m->m.alloc);

    m->tun_fd = -1;
    m->socket_fd = -1;
}

void main_free(main_t *m) {
    if (m->tun_fd >= 0) {
        close(m->tun_fd);
    }
    if (m->socket_fd >= 0) {
        close(m->socket_fd);
    }

    memscope_free(&m->m);
}

static const char *default_config_path = "/etc/ssl-tunnel/cfg.yaml";

int main(int argc, char *argv[]) {
    const char *path = getenv("CFG_PATH");
    if (path == NULL) {
        path = default_config_path;
    }

    main_t m;
    main_init(&m);

    err_t err = config_read(path, &m.cfg);
    if (!ERROR_OK(err)) {
        goto cleanup;
    }

    if (!ERROR_OK(err = fd_tun_open(m.cfg.interface.name, &m.tun_fd))) {
        goto cleanup;
    }

    if (!ERROR_OK(err = fd_set_nonblock(m.tun_fd))) {
        goto cleanup;
    }

    if (!ERROR_OK(err = fd_udp_open(&m.socket_fd))) {
        goto cleanup;
    }

    if (optional_is_some(m.cfg.interface.listen_port)) {
        if (!ERROR_OK(err = fd_udp_bind_local(m.socket_fd, optional_unwrap(m.cfg.interface.listen_port)))) {
            goto cleanup;
        }
    }

    if (!ERROR_OK(err = fd_set_nonblock(m.socket_fd))) {
        goto cleanup;
    }

    if (!ERROR_OK(err = fd_eventfd(&signal_fd))) {
        goto cleanup;
    }

    signal_init();

    if (!ERROR_OK(err = tunnel_main(&m.cfg, m.tun_fd, m.socket_fd, signal_fd))) {
        goto cleanup;
    }

cleanup:
    main_free(&m);

    if (signal_fd >= 0) {
        close(signal_fd);
    }

    if (!ERROR_OK(err)) {
        printf("Error: %s\n", err.msg);
        return errno;
    }

    return 0;
}
