#include <ssl-tunnel/signal.h>
#include <ssl-tunnel/memory.h>
#include <ssl-tunnel/errors.h>
#include <ssl-tunnel/flag.h>
#include <ssl-tunnel/fd.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/epoll.h>

typedef struct {
    int tun_fd;
    int server_fd;
    int poll_fd;
    volatile bool sig_received;
} server;

const err_t ERROR_IFCONFIG_FAILED = {
        .ok = false,
        .msg = "error executing ifconfig"
};

void _server_close(server srv) {
    close(srv.tun_fd);
    close(srv.server_fd);
    close(srv.poll_fd);
}

err_t _ifconfig_device_up() {
    int status = system("ifconfig tun0 10.8.0.1 10.8.0.2 mtu 1500 netmask 255.255.255.255 up");
    if (status != 0) {
        return ERROR_IFCONFIG_FAILED;
    }

    return ERROR_OK;
}

err_t _handle_socket_read() {
    printf("socket read\n");
    return ERROR_OK;
}

err_t _handle_tun_read() {
    printf("tun read\n");
    return ERROR_OK;
}

err_t server_main(int argc, char *argv[]) {
    alloc_pool_t p;
    alloc_pool_init(&p);

    err_t err;
    char *cfg_path;
    if (!ERR_OK(err = flag_parse(argc, argv, &cfg_path))) {
        return err;
    }

    server srv;
    memset(&srv, 0, sizeof(server));

    signal_init(&srv.sig_received);

    if (!ERR_OK(err = fd_tun_open("tun0", &srv.tun_fd))) {
        return err;
    }

    if (!ERR_OK(err = fd_set_nonblock(srv.tun_fd))) {
        return err;
    }

    if (!ERR_OK(err = _ifconfig_device_up())) {
        return err;
    }

    if (!ERR_OK(err = fd_udp_server_open(1026, &srv.server_fd))) {
        return err;
    }

    if (!ERR_OK(err = fd_set_nonblock(srv.server_fd))) {
        return err;
    }

    if (!ERR_OK(err = fd_poll_create(&srv.poll_fd))) {
        return err;
    }

    if (!ERR_OK(err = fd_poll_add(srv.poll_fd, srv.tun_fd, POLL_READ))) {
        return err;
    }

    if (!ERR_OK(err = fd_poll_add(srv.poll_fd, srv.server_fd, POLL_READ))) {
        return err;
    }

    struct epoll_event evs[2];

    printf("server listening on %d\n", 1026);
    while (!srv.sig_received) {
        int fd_ready;
        if (!ERR_OK(err = fd_poll_wait(srv.poll_fd, evs, 2, 0, &fd_ready))) {
            return err;
        }

        for (int i = 0; i < fd_ready; i++) {
            struct epoll_event ev = evs[i];
            int fd = ev.data.fd;

            if (fd == srv.server_fd) {
                _handle_socket_read();
                continue;
            }

            if (fd != srv.server_fd) {
                panicf("unexpected fd received");
            }

            _handle_tun_read();
        }
    }

    _server_close(srv);
    alloc_pool_free(&p);
    return ERROR_OK;
}