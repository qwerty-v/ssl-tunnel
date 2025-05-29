#include <ssl-tunnel/lib/memscope.h>
#include <ssl-tunnel/lib/err.h>
#include <ssl-tunnel/lib/fd.h>
#include <ssl-tunnel/lib/deque.h>
#include <ssl-tunnel/lib/optional.h>

#include <ssl-tunnel/backend/hashmap.h>
#include <ssl-tunnel/backend/trie.h>
#include <ssl-tunnel/backend/proto.h>
#include <ssl-tunnel/backend/config.h>

#include <stdio.h>
#include <errno.h>
#include <unistd.h> // close
#include <stdlib.h>
#include <stdatomic.h> // atomic_bool, atomic_load
#include <sys/epoll.h>
#include <netinet/ip.h>
#include <netinet/in.h> // struct sockaddr, socklen_t
#include <arpa/inet.h> // ntohl
#include <assert.h>
#include <time.h>

typedef struct {
    const config_peer_t *cfg_entry;

    uint32_t index;

    bool is_remote_addr_known;
    struct sockaddr_in remote_addr;
    socklen_t remote_addr_len;
} peer_t;

typedef struct {
    size_t packet_len;
    uint8_t packet_bytes[PROTO_MAX_MTU];

    peer_t *peer;
} queued_packet_t;

typedef deque_t(queued_packet_t) packet_queue_t;

typedef struct {
    memscope_t m;

    slice_t(peer_t) peers;
    hashmap_t index_lookup; // [uint32_t remote_index] -> peer_t *peer
    trie_t route_lookup;

    int poll_fd;
    packet_queue_t recv_q;
    packet_queue_t send_q;
} tunnel_t;

static void print_buffer(const uint8_t *arr, size_t arr_len) {
    for (size_t i = 0; i < arr_len; i++) {
        if (i != 0 && i % 8 == 0) {
            printf("\n");
        }
        printf("%02x ", arr[i]);
    }
    printf("\n");
}

static void lookup_route(const uint8_t *packet, size_t packet_len, const trie_t *t, peer_t **out_peer,
                         bool *out_ok) {
    if (packet_len < PROTO_IP_MIN_HEADER_LEN) {
        // invalid packet;
        *out_ok = false;
        return;
    }

    const struct ip *ip_packet = (const struct ip *) packet;
    uint32_t ip_dst = ntohl(ip_packet->ip_dst.s_addr);

    trie_match(t, ip_dst, (void **) out_peer, out_ok);
}

static void lookup_index(const uint8_t *packet, size_t packet_len, const hashmap_t *h, peer_t **out_peer,
                          bool *out_ok) {
    if (packet_len < PROTO_TRANSPORT_HEADER_LEN) {
        // invalid packet;
        *out_ok = false;
        return;
    }

    const proto_transport_t *transport_packet = (const proto_transport_t *) packet;

    hashmap_get(h, transport_packet->remote_index, (void **) out_peer, out_ok);
}

static void io_udp_read(int socket_fd, packet_queue_t *recv_q, const hashmap_t *index_lookup) {
    // socket -> recv_q
    while (1) {
        queued_packet_t p;
        memset(&p, 0, sizeof(queued_packet_t));

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(struct sockaddr_in));
        addr.sin_family = AF_INET;

        socklen_t addr_len;

        ssize_t n = recvfrom(socket_fd, p.packet_bytes, PROTO_MAX_MTU, 0, (struct sockaddr *) &addr, &addr_len);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            panicf("error calling recvfrom (errno %d)", errno);
        }
        p.packet_len = n;

        assert(addr.sa_family == AF_INET);
        printf("%d\tio_udp_read():\tfrom: %s\trecv_q.len: %lu\tbytes: %lu\n", (int)time(NULL), inet_ntoa(addr.sin_addr), recv_q->len, p.packet_len);
        print_buffer(p.packet_bytes, p.packet_len);

        bool ok;
        lookup_index(p.packet_bytes, p.packet_len, index_lookup, &p.peer, &ok);
        if (!ok) {
            // unknown peer; skip
            continue;
        }

        if (!p.peer->is_remote_addr_known) {
            p.peer->remote_addr = addr;
            p.peer->remote_addr_len = addr_len;

            p.peer->is_remote_addr_known = true;
        }

        deque_push_back((deque_any_t *) recv_q, &p);
    }
}

static void io_tun_write(int tun_fd, packet_queue_t *recv_q) {
    // recv_q -> tun
    while (recv_q->len > 0) {
        queued_packet_t p;
        memset(&p, 0, sizeof(queued_packet_t));

        err_t err = deque_front((deque_any_t *) recv_q, &p);
        if (!ERROR_OK(err)) {
            panicf("error reading front of the recv_q queue: %s", err.msg);
        }

        const proto_transport_t *transport_packet = (const proto_transport_t *) p.packet_bytes;

        if (p.packet_len <= PROTO_TRANSPORT_HEADER_LEN) {
            panicf("packet len too small");
        }
        size_t len = p.packet_len - PROTO_TRANSPORT_HEADER_LEN;

        assert(p.peer->remote_addr.sin_family == AF_INET);
        printf("%d\tio_tun_write():\tp.peer.remote_addr: %s\trecv_q.len: %lu\tbytes: %lu\n", (int)time(NULL), inet_ntoa(p.peer->remote_addr.sin_addr), recv_q->len, len);
        print_buffer(transport_packet->data, len);

        ssize_t n = write(tun_fd, transport_packet->data, len);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            panicf("error calling write (errno %d)", errno);
        }
        if ((size_t) n != len) {
            panicf("unable to write whole buffer into device");
        }

        if (!ERROR_OK(err = deque_pop_front((deque_any_t *) recv_q))) {
            panicf("error removing queue element");
        }
    }
}

static void io_tun_read(int tun_fd, packet_queue_t *send_q, const trie_t *route_lookup) {
    // tun -> send_q
    while (1) {
        queued_packet_t p;
        memset(&p, 0, sizeof(queued_packet_t));

        ssize_t n = read(tun_fd, p.packet_bytes, PROTO_MAX_MTU);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            panicf("error calling read (errno %d)", errno);
        }
        p.packet_len = n;

        printf("%d\tio_tun_read():\tsend_q.len: %lu\tbytes: %lu\n", (int) time(NULL), send_q->len, p.packet_len);
        print_buffer(p.packet_bytes, p.packet_len);

        bool ok;
        lookup_route(p.packet_bytes, p.packet_len, route_lookup, &p.peer, &ok);
        if (!ok) {
            // route hasn't been matched; skip
            continue;
        }

        deque_push_back((deque_any_t *) send_q, &p);
    }
}

static void io_udp_write(int socket_fd, packet_queue_t *send_q, uint32_t self_index) {
    // send_q -> udp
    while (send_q->len > 0) {
        queued_packet_t p;
        memset(&p, 0, sizeof(queued_packet_t));

        err_t err = deque_front((deque_any_t *) send_q, &p);
        if (!ERROR_OK(err)) {
            panicf("error reading front of the send_q queue: %s", err.msg);
        }

        if (!p.peer->is_remote_addr_known) {
            // skip;
            deque_pop_front((deque_any_t *) send_q);
            continue;
        }

        size_t len;
        proto_transport_t packet;
        err = proto_new_transport_packet(self_index, 0, p.packet_bytes,
                                         p.packet_len, &packet, &len);
        if (!ERROR_OK(err)) {
            panicf("error creating new packet: %s", err.msg);
        }

        assert(p.peer->remote_addr.sin_family == AF_INET);
        printf("%d\tio_udp_write():\tp.peer.remote_addr: %s\tsend_q.len: %lu\tbytes: %lu\n", (int) time(NULL), inet_ntoa(p.peer->remote_addr.sin_addr), send_q->len, len);
        print_buffer((uint8_t *) &packet, len);

        ssize_t n = sendto(socket_fd, &packet, len, 0, (struct sockaddr *) &p.peer->remote_addr, p.peer->remote_addr_len);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            panicf("error calling sendto (errno %d)", errno);
        }
        if ((size_t) n != len) {
            panicf("bytes sent and total buffered bytes did not match");
        }

        if (!ERROR_OK(err = deque_pop_front((deque_any_t *) send_q))) {
            panicf("error popping front of send_q");
        }
    }
}

static void tunnel_handle_socket_read(tunnel_t *t, int tun_fd, int socket_fd) {
    io_udp_read(socket_fd, &t->recv_q, &t->index_lookup);

    io_tun_write(tun_fd, &t->recv_q);

    if (t->recv_q.len > 0) {
        fprintf(stderr, "warn: device busy\n");
    }
}

static void tunnel_handle_tun_read(tunnel_t *t, int tun_fd, int socket_fd, uint32_t self_index) {
    io_tun_read(tun_fd, &t->send_q, &t->route_lookup);

    io_udp_write(socket_fd, &t->send_q, self_index);

    if (t->send_q.len > 0) {
        fprintf(stderr, "warn: socket busy\n");
    }
}

static void tunnel_init(tunnel_t *t) {
    memset(t, 0, sizeof(tunnel_t));

    memscope_init(&t->m);

    slice_init((slice_any_t *) &t->peers, sizeof(peer_t), &t->m.alloc);
    hashmap_init(&t->index_lookup, &t->m.alloc);
    trie_init(&t->route_lookup, &t->m.alloc);

    t->poll_fd = -1;
    deque_init((deque_any_t *) &t->recv_q, sizeof(queued_packet_t), &t->m.alloc);
    deque_init((deque_any_t *) &t->send_q, sizeof(queued_packet_t), &t->m.alloc);
}

static void tunnel_free(tunnel_t *t) {
    if (t->poll_fd >= 0) {
        close(t->poll_fd);
    }

    memscope_free(&t->m);
}

err_t tunnel_event_loop(const config_t *cfg, int tun_fd, int socket_fd, const volatile atomic_bool *flag) {
    tunnel_t t;
    tunnel_init(&t);

    for (int i = 0; i < (int) cfg->peers.len; i++) {
        peer_t p;
        memset(&p, 0, sizeof(peer_t));

        const config_peer_t *cfg_entry = &cfg->peers.array[i];

        p.cfg_entry = cfg_entry;
        p.index = cfg_entry->index;

        p.is_remote_addr_known = optional_is_some(p.cfg_entry->remote);
        if (p.is_remote_addr_known) {
            socklen_t len = sizeof(struct sockaddr_in);

            memcpy(&p.remote_addr, &cfg_entry->remote.v, len);
            p.remote_addr_len = len;
        }

        slice_append((slice_any_t *) &t.peers, &p);
    }

    for (int i = 0; i < (int) t.peers.len; i++) {
        peer_t *p = &t.peers.array[i];

        hashmap_insert(&t.index_lookup, p->index, (void **) &p);

        trie_insert(&t.route_lookup, p->cfg_entry->addr, p->cfg_entry->addr_prefix, (void **) &p);
    }

    err_t err;
    if (!ERROR_OK(err = fd_poll_create(&t.poll_fd))) {
        goto cleanup;
    }

    if (!ERROR_OK(err = fd_poll_add(t.poll_fd, tun_fd, FD_POLL_READ))) {
        goto cleanup;
    }

    if (!ERROR_OK(err = fd_poll_add(t.poll_fd, socket_fd, FD_POLL_READ))) {
        goto cleanup;
    }

    struct epoll_event evs[2];

    if (optional_is_some(cfg->interface.listen_port)) {
        printf("server is listening on port %d\n", optional_unwrap(cfg->interface.listen_port));
        fflush(stdout);
    }

    while (atomic_load(flag)) {
        int fd_ready;
        err = fd_poll_wait(t.poll_fd, evs, 2, 0, &fd_ready);
        if (!ERROR_OK(err)) {
            goto cleanup;
        }

        for (int i = 0; i < fd_ready; i++) {
            struct epoll_event ev = evs[i];
            int fd = ev.data.fd;

            if (fd == tun_fd) {
                tunnel_handle_tun_read(&t, tun_fd, socket_fd, cfg->interface.index);
                continue;
            }

            tunnel_handle_socket_read(&t, tun_fd, socket_fd);
        }
    }
    printf("signal received, exiting...\n");

cleanup:
    tunnel_free(&t);
    return err;
}