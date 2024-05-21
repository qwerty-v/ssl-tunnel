#include <ssl-tunnel/errors.h>

#include <linux/if.h>
#include <linux/if_tun.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/epoll.h>

const err_t ERROR_OPEN_FAILED = {
        .ok = false,
        .msg = "error calling open"
};

const err_t ERROR_IOCTL_FAILED = {
        .ok = false,
        .msg = "error calling ioctl"
};

const err_t ERROR_FCNTL_FAILED = {
        .ok = false,
        .msg = "error calling ioctl"
};

const err_t ERROR_SOCKET_FAILED = {
        .ok = false,
        .msg = "error calling socket"
};

const err_t ERROR_BIND_FAILED = {
        .ok = false,
        .msg = "error calling bind"
};

const err_t ERROR_EPOLL_CREATE_FAILED = {
        .ok = false,
        .msg = "error creating epoll"
};

const err_t ERROR_EPOLL_CTL_FAILED = {
        .ok = false,
        .msg = "error calling epoll_ctl"
};

const err_t ERROR_EPOLL_WAIT_FAILED = {
        .ok = false,
        .msg = "error calling epoll_wait"
};

const int POLL_READ = 1 << 0;
const int POLL_WRITE = 1 << 1;

err_t fd_tun_open(const char *device_name, int *tun_fd) {
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(struct ifreq));

    // no packet info (IFF_NO_PI), tun device (IFF_TUN)
    ifr.ifr_flags = IFF_NO_PI | IFF_TUN;
    strncpy(ifr.ifr_name, device_name, IFNAMSIZ);

    int fd = open("/dev/net/tun", O_RDWR);
    if (fd < 0) {
        return ERROR_OPEN_FAILED;
    }

    if (ioctl(fd, TUNSETIFF, (void *) &ifr) < 0) {
        return ERROR_IOCTL_FAILED;
    }

    *tun_fd = fd;
    return ERROR_OK;
}

err_t fd_udp_server_open(int port, int *server_fd) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        return ERROR_SOCKET_FAILED;
    }

    struct sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_port = htons(port);
    local.sin_addr.s_addr = INADDR_ANY;

    if (bind(fd, (const struct sockaddr*) &local, sizeof(struct sockaddr_in)) < 0) {
        return ERROR_BIND_FAILED;
    }

    *server_fd = fd;
    return ERROR_OK;
}

err_t fd_poll_create(int *poll_fd) {
    int fd = epoll_create1(0);
    if (fd < 0) {
        return ERROR_EPOLL_CREATE_FAILED;
    }

    *poll_fd = fd;
    return ERROR_OK;
}

err_t fd_poll_add(int poll_fd, int fd, int flags) {
    struct epoll_event ev;

    int f = 0;
    if (flags & POLL_READ) {
        f |= EPOLLIN;
    }
    if (flags & POLL_WRITE) {
        f |= EPOLLOUT;
    }

    ev.events = f;
    ev.data.fd = fd;

    if (epoll_ctl(poll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        return ERROR_EPOLL_CTL_FAILED;
    }

    return ERROR_OK;
}

err_t fd_poll_wait(int poll_fd, void *events, int max_events, int timeout_ms, int *out_fd_ready) {
    int n = epoll_wait(poll_fd, (struct epoll_event *) events, max_events, timeout_ms);
    if (n < 0) {
        return ERROR_EPOLL_WAIT_FAILED;
    }

    *out_fd_ready = n;
    return ERROR_OK;
}

err_t fd_set_nonblock(int fd) {
    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
        return ERROR_FCNTL_FAILED;
    }

    return ERROR_OK;
}