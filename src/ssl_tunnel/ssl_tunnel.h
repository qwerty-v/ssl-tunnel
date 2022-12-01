#ifndef SSL_TUNNEL_SSL_TUNNEL_H
#define SSL_TUNNEL_SSL_TUNNEL_H

#include "buffer.h"

#include <stdbool.h>

/**
 * Top-level context
 */
struct context {
    int port;
    bool running;

    int tun_fd;    /**< Tun interface file descriptor */
    int server_sd; /**< Server socket descriptor */
    int poll_fd;
};

/**
 * Context used in the main ssl_tunnel event loop
 */
struct ev_loop_context {
    struct buffer send_buf; /**< Buffer containing data read from tun_fd */
    struct buffer recv_buf; /**< Buffer containing data read from server_sd */

    struct session *sessions;
};

struct session {
    // ? real_addr
    // ? virtual_addr
};

#endif //SSL_TUNNEL_SSL_TUNNEL_H
