#include "ssl_tunnel.h"
#include "fd.h"

#include <sys/socket.h>
#include <netinet/in.h>

int socket_create_dgram() {
    return socket(AF_INET, SOCK_DGRAM, 0);
}

int socket_bind_local(int socket, int port) {
    struct sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_port = htons(port);
    local.sin_addr.s_addr = INADDR_ANY;

    return bind(socket, (const struct sockaddr*) &local, sizeof(local));
}

int socket_create_server(const struct context *ctx) {
    int socket = socket_create_dgram();
    if (socket < 0) {
        return -1;
    }

    if (socket_bind_local(socket, ctx->port) < 0) {
        return -1;
    }

    if (fd_set_nonblocking(socket) < 0) {
        return -1;
    }

    if (fd_set_cloexec(socket) < 0) {
        return -1;
    }

    return socket;
}