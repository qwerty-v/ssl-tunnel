#include <ssl-tunnel/lib/deque.h>
#include <ssl-tunnel/lib/proto.h>

#include <netinet/in.h>

typedef struct {
    int len;
    uint8_t packet[PROTO_MAX_MTU];
} io_wire_t;

void io_udp_read(int socket_fd, deque_t *recv_buf, struct sockaddr *addr, socklen_t *addr_len);

void io_tun_write(int tun_fd, deque_t *recv_buf);

void io_tun_read(int tun_fd, deque_t *send_buf);

void io_udp_write(int server_fd, deque_t *send_buf, const struct sockaddr *addr, socklen_t addr_len);
