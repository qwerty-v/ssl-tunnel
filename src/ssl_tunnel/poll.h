#ifndef SSL_TUNNEL_POLL_H
#define SSL_TUNNEL_POLL_H

#include "target.h"

#ifdef TARGET_APPLE

#include <sys/event.h>
#define POLL_READ EVFILT_READ
#define POLL_WRITE EVFILT_WRITE

#elifdef TARGET_LINUX

#include <sys/epoll.h>
#define POLL_READ EPOLLIN

#else
#error "not implemented"
#endif

int poll_create();

int poll_add(int kq, int fd, int event);

int poll_wait(int kq);

#endif //SSL_TUNNEL_POLL_H
