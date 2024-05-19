#include <ssl-tunnel/errors.h>

#include <linux/if.h>
#include <linux/if_tun.h>
#include <fcntl.h>
#include <sys/ioctl.h>

const err_t ERROR_TUN_OPEN = {
        .ok = false,
        .msg = "error opening tun device"
};

const err_t ERROR_IOCTL_FAILED = {
        .ok = false,
        .msg = "error executing ioctl"
};

const err_t ERROR_FCNTL_FAILED = {
        .ok = false,
        .msg = "error executing ioctl"
};

err_t fd_open_tun(const char *device_name, int *tun_fd) {
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(struct ifreq));

    // no packet info (IFF_NO_PI), tun device (IFF_TUN)
    ifr.ifr_flags = IFF_NO_PI | IFF_TUN;
    strncpy(ifr.ifr_name, device_name, IFNAMSIZ);

    int fd = open("/dev/net/tun", O_RDWR);
    if (fd < 0) {
        return ERROR_TUN_OPEN;
    }

    if (ioctl(fd, TUNSETIFF, (void *) &ifr) < 0) {
        return ERROR_IOCTL_FAILED;
    }

    *tun_fd = fd;
    return ERROR_OK;
}

err_t fd_set_nonblock(int fd) {
    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
        return ERROR_FCNTL_FAILED;
    }

    return ERROR_OK;
}