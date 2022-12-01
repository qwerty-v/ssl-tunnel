#include "target.h"
#include "poll.h"
#include "fd.h"

const int poll_max_fds = 1000;

#ifdef TARGET_APPLE

#include <stdlib.h>

int poll_create() {
    int kq = kqueue();

    if (fd_set_cloexec(kq) < 0) {
        return -1;
    }

    return kq;
}

int poll_add(int kq, int fd, int event) {
    struct kevent ev;
    EV_SET(&ev, fd, event, EV_ADD, 0, 0, 0);
    return kevent(kq, &ev, 1, NULL, 0, NULL);
}

int poll_wait(int kq) {
    return -1; // todo
}

#elifdef TARGET_LINUX

int poll_create() {
    return epoll_create1(0);
}

int poll_add(int poll, int fd, int event) {
    return -1; // todo
}

int poll_wait(int poll) {
    return -1; // todo
}

#else
#error "not implemented"
#endif