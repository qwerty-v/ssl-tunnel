#include <ssl-tunnel/backend/tunnel_send.h>

#include <ssl-tunnel/lib/err.h>

#include <stdio.h>
#include <errno.h>
#include <unistd.h> // write
#include <arpa/inet.h> // ntohl

static void lookup_route(const proto_ip_t *packet, size_t packet_len, const trie_t *t,
                         peer_t **out_peer, bool *out_ok) {
    if (packet_len < PROTO_HEADER_IP_MIN_LEN) {
        // invalid packet;
        *out_ok = false;
        return;
    }

    trie_match(t, ntohl(packet->dst), out_peer, out_ok);
}

void io_tun_read(int tun_fd, outbound_queue_t *send_q, const trie_t *route_lookup) {
    // tun -> send_q
    while (1) {
        size_t back;
        deque_prepare_push_back((deque_any_t *) send_q, &back);

        outbound_packet_t *p = &send_q->array[back];

        ssize_t n = read(tun_fd, &p->packet, PROTO_MAX_MTU);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            panicf("error calling read (errno %d)", errno);
        }
        p->packet_len = n;

        bool ok;
        lookup_route(&p->packet, p->packet_len, route_lookup, &p->peer, &ok);
        if (!ok) {
            // route hasn't been matched; skip
            continue;
        }

        send_q->back = back;
        send_q->len++;
    }
}

void io_udp_write(int socket_fd, outbound_queue_t *send_q, uint32_t self_index) {
    // send_q -> udp
    while (send_q->len > 0) {
        err_t err;

        outbound_packet_t *p = &send_q->array[send_q->front];
        if (!p->peer->is_remote_addr_known) {
            goto next;
        }

        size_t len;
        proto_transport_t packet;
        err = proto_new_transport_packet(self_index, 0, &p->packet, p->packet_len, &packet, &len);
        if (!ERROR_OK(err)) {
            printf("error creating new packet: %s (%lu)", err.msg, p->packet_len);
            goto next;
        }

        ssize_t n = sendto(socket_fd, &packet, len, 0, (struct sockaddr *) &p->peer->remote_addr, p->peer->remote_addr_len);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            panicf("error calling sendto (errno %d)", errno);
        }
        if ((size_t) n != len) {
            panicf("bytes sent and total buffered bytes did not match");
        }

    next:
        if (!ERROR_OK(err = deque_pop_front((deque_any_t *) send_q))) {
            panicf("error popping front of send_q: %s", err.msg);
        }
    }
}
