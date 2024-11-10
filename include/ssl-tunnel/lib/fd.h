#pragma once

#include <ssl-tunnel/lib/err.h> // err_t

extern const int POLL_READ;
extern const int POLL_WRITE;

err_t fd_tun_open(const char *device_name, int *tun_fd);

err_t fd_udp_server_open(int port, int *server_fd);

err_t fd_poll_create(int *poll_fd);

err_t fd_poll_add(int poll_fd, int fd, int flags);

err_t fd_poll_wait(int poll_fd, void *events, int max_events, int timeout_ms, int *out_fd_ready);

err_t fd_set_nonblock(int fd);
