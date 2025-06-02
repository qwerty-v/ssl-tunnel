#include <ssl-tunnel/backend/tunnel_recv.h>

#include <ssl-tunnel/lib/err.h>

#include <errno.h>
#include <unistd.h> // write

static void lookup_index(const proto_transport_t *packet, size_t packet_len, const hashmap_t *h,
                         peer_t **out_peer, bool *out_ok) {
    if (packet_len < PROTO_HEADER_TRANSPORT_LEN) {
        // invalid packet;
        *out_ok = false;
        return;
    }

    hashmap_get(h, packet->remote_index, out_peer, out_ok);
}

void io_udp_read(int socket_fd, inbound_queue_t *recv_q, const hashmap_t *index_lookup) {
    // socket -> recv_q
    while (1) {
        size_t back;
        deque_prepare_push_back((deque_any_t *) recv_q, &back);

        inbound_packet_t *p = &recv_q->array[back];

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(struct sockaddr_in));

        socklen_t addr_len = sizeof(struct sockaddr_in);

        ssize_t n = recvfrom(socket_fd, &p->packet, PROTO_MAX_MTU, 0, (struct sockaddr *) &addr, &addr_len);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            panicf("error calling recvfrom (errno %d)", errno);
        }
        p->packet_len = n;

        bool ok;
        lookup_index(&p->packet, p->packet_len, index_lookup, &p->peer, &ok);
        if (!ok) {
            // unknown peer; skip
            continue;
        }

        if (!p->peer->is_remote_addr_known) {
            p->peer->remote_addr = addr;
            p->peer->remote_addr_len = addr_len;

            p->peer->is_remote_addr_known = true;
        }

        recv_q->back = back;
        recv_q->len++;
    }
}

void io_tun_write(int tun_fd, inbound_queue_t *recv_q) {
    // recv_q -> tun
    while(recv_q->len > 0) {
        const inbound_packet_t *p = &recv_q->array[recv_q->front];
        if (p->packet_len <= PROTO_HEADER_TRANSPORT_LEN) {
            panicf("packet len too small");
        }

        size_t len = p->packet_len - PROTO_HEADER_TRANSPORT_LEN;

        ssize_t n = write(tun_fd, p->packet.data, len);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            panicf("error calling write (errno %d)", errno);
        }
        if ((size_t) n != len) {
            panicf("unable to write whole buffer into device");
        }

        err_t err = deque_pop_front((deque_any_t *) recv_q);
        if (!ERROR_OK(err)) {
            panicf("error removing queue element");
        }
    }
}
