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

void io_tun_read(int tun_fd, outbound_queue_t *send_q, pthread_mutex_t *mx, const trie_t *route_lookup, outbound_packet_t *arr) {
    // tun -> send_q
    int len = 0;

    while (len < OUTBOUND_BUF_SIZE) {
        outbound_packet_t *p = &arr[len];

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

        len++;
    }

    if (len == 0) {
        return;
    }

    if (len == OUTBOUND_BUF_SIZE) {
        printf("tun_full\n");
        fflush(stdout);
    }

    pthread_mutex_lock(mx);
    for (int i = 0; i < len; i++) {
        deque_push_back((deque_any_t *) send_q, &arr[i]);
    }
    pthread_mutex_unlock(mx);
}

void io_udp_write(int socket_fd, outbound_queue_t *send_q, pthread_mutex_t *mx, uint32_t self_index, outbound_packet_t *arr) {
    // send_q -> udp
    int len = 0;

    pthread_mutex_lock(mx);
    while (len < OUTBOUND_BUF_SIZE) {
        if (send_q->len == 0) {
            break;
        }

        err_t err;
        if (!ERROR_OK(err = deque_front((deque_any_t *) send_q, &arr[len]))) {
            panicf("unable to pop front: %s", err.msg);
        }

        if (!ERROR_OK(err = deque_pop_front((deque_any_t *) send_q))) {
            panicf("error popping front of send_q: %s", err.msg);
        }

        if (!arr[len].peer->is_remote_addr_known) {
            continue;
        }

        len++;
    }
    pthread_mutex_unlock(mx);

    for (int i = 0; i < len; i++) {
        outbound_packet_t *p = &arr[i];

        size_t write_len;
        proto_transport_t packet;
        err_t err = proto_new_transport_packet(self_index, 0, (const uint8_t *) &p->packet, p->packet_len, &packet, &write_len);
        if (!ERROR_OK(err)) {
            printf("error creating new packet: %s (%lu)", err.msg, p->packet_len);
            continue;
        }

        ssize_t n = sendto(socket_fd, &packet, write_len, 0, (struct sockaddr *) &p->peer->remote_addr,
                           p->peer->remote_addr_len);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("warn: udp dropping packets!!!\n");
                fflush(stdout);
                break;
            }

            panicf("error calling sendto (errno %d)", errno);
        }
        if ((size_t) n != write_len) {
            panicf("bytes sent and total buffered bytes did not match");
        }
    }
}
