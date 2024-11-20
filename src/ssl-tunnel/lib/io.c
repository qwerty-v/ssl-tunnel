#include <ssl-tunnel/lib/io.h>
#include <ssl-tunnel/lib/proto.h>

#include <errno.h>
#include <unistd.h>

void io_udp_read(int socket_fd, deque_t *recv_buf, struct sockaddr *addr, socklen_t *addr_len) {
    // socket -> recv_buf
    for (io_wire_t d; 1; ) {
        d.len = recvfrom(socket_fd, d.packet, PROTO_MAX_MTU, 0, addr, addr_len);
        if (d.len < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            panicf("error calling recvfrom (errno %d)", errno);
        }

        deque_push_back(recv_buf, &d);
    }
}

void io_tun_write(int tun_fd, deque_t *recv_buf) {
    // recv_buf -> tun
    for (io_wire_t d; recv_buf->len > 0; ) {
        err_t err = deque_pop_front(recv_buf, &d);
        if (!ERROR_OK(err)) {
            panicf("error reading front of the recv_buf queue: %s", err.msg);
        }

        int n = write(tun_fd, d.packet, d.len);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            panicf("error calling write (errno %d)", errno);
        }
        if (n != d.len) {
            panicf("unable to write whole buffer into device");
        }
    }
}

void io_tun_read(int tun_fd, deque_t *send_buf) {
    // tun -> send_buf
    for (io_wire_t d; 1; ) {
        d.len = read(tun_fd, d.packet, PROTO_MAX_MTU);
        if (d.len < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            panicf("error calling read (errno %d)", errno);
        }

        deque_push_back(send_buf, &d);
    }
}

void io_udp_write(int server_fd, deque_t *send_buf, const struct sockaddr *addr, socklen_t addr_len) {
    // send_buf -> udp
    for (io_wire_t d; send_buf->len > 0; ) {
        err_t err = deque_pop_front(send_buf, &d);
        if (!ERROR_OK(err)) {
            panicf("error reading front of the send_buf queue: %s", err.msg);
        }

        int n = sendto(server_fd, d.packet, d.len, 0, addr, addr_len);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            panicf("error calling sendto (errno %d)", errno);
        }
        if (n != d.len) {
            panicf("could not send whole buffer to client");
        }
    }
}
