#pragma once

#include <ssl-tunnel/lib/err.h> // err_t
#include <netinet/in.h> // sockaddr

extern const int FD_POLL_READ;
extern const int FD_POLL_WRITE;
extern const int FD_POLL_EDGE_TRIGGERED;

err_t fd_tun_open(const char *device_name, int *out_tun_fd);

err_t fd_udp_open(int *out_fd);

err_t fd_udp_bind_local(int fd, int port);

err_t fd_poll_create(int *out_poll_fd);

err_t fd_poll_add(int poll_fd, int fd, int flags);

err_t fd_poll_wait(int poll_fd, void *events, int max_events, int timeout_ms, int *out_fd_ready);

err_t fd_set_nonblock(int fd);

err_t fd_eventfd(int *out_fd);
