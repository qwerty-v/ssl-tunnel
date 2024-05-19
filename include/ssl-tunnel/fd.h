#include <ssl-tunnel/errors.h>

extern const err_t ERROR_TUN_OPEN;
extern const err_t ERROR_IOCTL_FAILED;
extern const err_t ERROR_FCNTL_FAILED;

err_t fd_open_tun(const char *device_name, int *tun_fd);
err_t fd_set_nonblock(int fd);