#include <ssl-tunnel/backend/tunnel_recv.h>

#include <ssl-tunnel/lib/err.h>

#include <errno.h>
#include <unistd.h> // write
#include <stdio.h> // fprintf

static void lookup_index(const proto_transport_t *packet, size_t packet_len, const hashmap_t *h,
                         peer_t **out_peer, bool *out_ok) {
    if (packet_len < PROTO_HEADER_TRANSPORT_LEN) {
        // invalid packet;
        *out_ok = false;
        return;
    }

    hashmap_get(h, packet->remote_index, out_peer, out_ok);
}

void io_udp_read(int socket_fd, inbound_queue_t *recv_q, pthread_mutex_t *mx, const hashmap_t *index_lookup, inbound_packet_t *arr) {
    // socket -> recv_q
    struct sockaddr_in addrs[INBOUND_BUF_SIZE];
    int len = 0;

    while (len < INBOUND_BUF_SIZE) {
        inbound_packet_t *p = &arr[len];

        socklen_t addr_len = sizeof(struct sockaddr_in);

        ssize_t n = recvfrom(socket_fd, &p->packet, PROTO_MAX_MTU, 0, (struct sockaddr *) &addrs[len], &addr_len);
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

        len++;
    }

    if (len == 0) {
        return;
    }

    if (len == INBOUND_BUF_SIZE) {
        printf("udp_full\n");
        fflush(stdout);
    }

    pthread_mutex_lock(mx);
    for (int i = 0; i < len; i++) {
        inbound_packet_t *p = &arr[i];

        if (!p->peer->is_remote_addr_known) {
            p->peer->remote_addr = addrs[i];
            p->peer->remote_addr_len = sizeof(struct sockaddr_in);

            p->peer->is_remote_addr_known = true;
        }

        deque_push_back((deque_any_t *) recv_q, p);
    }
    pthread_mutex_unlock(mx);
}

void io_tun_write(int tun_fd, inbound_queue_t *recv_q, pthread_mutex_t *mx, inbound_packet_t *arr) {
    // recv_q -> tun
    int len = 0;

    pthread_mutex_lock(mx);
    for (int i = 0; i < INBOUND_BUF_SIZE; i++) {
        if (recv_q->len == 0) {
            break;
        }

        err_t err = deque_front((deque_any_t *) recv_q, &arr[i]);
        if (!ERROR_OK(err)) {
            panicf("unable to pop front: %s", err.msg);
        }

        if (arr[i].packet_len <= PROTO_HEADER_TRANSPORT_LEN) {
            panicf("packet len too small");
        }

        if (!ERROR_OK(err = deque_pop_front((deque_any_t *) recv_q))) {
            panicf("error removing queue element");
        }

        len++;
    }
    pthread_mutex_unlock(mx);

    for (int i = 0; i < len; i++) {
        size_t write_len = arr[i].packet_len - PROTO_HEADER_TRANSPORT_LEN;

        ssize_t n = write(tun_fd, arr[i].packet.data, write_len);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("warn: tun dropping packets!!!\n");
                fflush(stdout);
                break;
            }

            panicf("error calling write (errno %d)", errno);
        }
        if ((size_t) n != write_len) {
            panicf("unable to write whole buffer into device");
        }
    }
}
