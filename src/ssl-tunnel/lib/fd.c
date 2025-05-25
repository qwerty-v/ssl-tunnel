#include <ssl-tunnel/lib/err.h>

#include <linux/if.h> // ifreq, IFNAMSIZ
#include <linux/if_tun.h> // IFF_NO_PI, IFF_TUN, TUNSETIFF
#include <fcntl.h> // open, O_RDWR, fcntl, F_SETFL, O_NONBLOCK
#include <netinet/in.h> // sockaddr_in, htons, INADDR_ANY
#include <sys/ioctl.h> // ioctl
#include <sys/socket.h> // socket
#include <sys/epoll.h> // epoll_create1

const int FD_POLL_READ = 1 << 0;
const int FD_POLL_WRITE = 1 << 1;

err_t fd_tun_open(const char *device_name, int *out_tun_fd) {
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(struct ifreq));

    // no packet info (IFF_NO_PI), tun device (IFF_TUN)
    ifr.ifr_flags = IFF_NO_PI | IFF_TUN;
    strncpy(ifr.ifr_name, device_name, IFNAMSIZ);

    int fd = open("/dev/net/tun", O_RDWR);
    if (fd < 0) {
        return err_errno();
    }

    if (ioctl(fd, TUNSETIFF, (void *) &ifr) < 0) {
        return err_errno();
    }

    *out_tun_fd = fd;
    return ENULL;
}

err_t fd_udp_open(int *out_fd) {
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) {
        return err_errno();
    }

    *out_fd = fd;
    return ENULL;
}

err_t fd_udp_bind_local(int fd, int port) {
    struct sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_port = htons(port);
    local.sin_addr.s_addr = INADDR_ANY;

    if (bind(fd, (const struct sockaddr*) &local, sizeof(struct sockaddr_in)) < 0) {
        return err_errno();
    }

    return ENULL;
}

err_t fd_poll_create(int *out_poll_fd) {
    int fd = epoll_create1(0);
    if (fd < 0) {
        return err_errno();
    }

    *out_poll_fd = fd;
    return ENULL;
}

err_t fd_poll_add(int poll_fd, int fd, int flags) {
    struct epoll_event ev;

    int f = 0;
    if (flags & FD_POLL_READ) {
        f |= EPOLLIN;
    }
    if (flags & FD_POLL_WRITE) {
        f |= EPOLLOUT;
    }

    ev.events = f;
    ev.data.fd = fd;

    if (epoll_ctl(poll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        return err_errno();
    }

    return ENULL;
}

err_t fd_poll_wait(int poll_fd, void *events, int max_events, int timeout_ms, int *out_fd_ready) {
    int n = epoll_wait(poll_fd, (struct epoll_event *) events, max_events, timeout_ms);
    if (n < 0) {
        return err_errno();
    }

    *out_fd_ready = n;
    return ENULL;
}

err_t fd_set_nonblock(int fd) {
    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
        return err_errno();
    }

    return ENULL;
}
