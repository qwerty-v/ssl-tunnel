#include <ssl-tunnel/errors.h>

extern const int POLL_READ;
extern const int POLL_WRITE;

extern const err_t ERROR_OPEN_TUN;
extern const err_t ERROR_IOCTL_FAILED;
extern const err_t ERROR_FCNTL_FAILED;
extern const err_t ERROR_CREATE_SOCKET;
extern const err_t ERROR_BIND_FAILED;

err_t fd_tun_open(const char *device_name, int *tun_fd);
err_t fd_udp_server_open(int port, int *server_fd);
err_t fd_poll_create(int *poll_fd);
err_t fd_poll_add(int poll_fd, int fd, int flags);
err_t fd_poll_wait(int poll_fd, void *events, int max_events, int timeout_ms, int *out_fd_ready);
err_t fd_set_nonblock(int fd);