#include <sys/fcntl.h>

int fd_set_nonblocking(int fd) {
    return fcntl(fd, F_SETFL, O_NONBLOCK);
}

int fd_set_cloexec(int fd) {
    return fcntl(fd, F_SETFD, FD_CLOEXEC);
}