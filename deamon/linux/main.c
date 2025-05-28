#include <ssl-tunnel/backend/config.h>
#include <ssl-tunnel/backend/tunnel.h>

#include <ssl-tunnel/lib/memscope.h>
#include <ssl-tunnel/lib/fd.h>

#include <stdlib.h> // getenv
#include <stdio.h> // printf
#include <errno.h> // errno
#include <stdbool.h> // true, false
#include <stdatomic.h> // atomic_bool, atomic_store, atomic_init
#include <signal.h> // signal
#include <unistd.h> // close

static const char *default_config_path = "/etc/ssl-tunnel/cfg.yaml";

static volatile atomic_bool signal_flag;

static void signal_handler(int unused) {
    (void) unused;
    atomic_store(&signal_flag, false);
}

void signal_init() {
    atomic_init(&signal_flag, true);

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

    signal_init();

    if (!ERROR_OK(err = tunnel_event_loop(&m.cfg, m.tun_fd, m.socket_fd, &signal_flag))) {
        goto cleanup;
    }

cleanup:
    main_free(&m);

    if (!ERROR_OK(err)) {
        printf("Error: %s\n", err.msg);
        return errno;
    }

    return 0;
}