#include <ssl-tunnel/lib/memscope.h>
#include <ssl-tunnel/lib/err.h>
#include <ssl-tunnel/lib/fd.h>
#include <ssl-tunnel/lib/deque.h>
#include <ssl-tunnel/lib/optional.h>

#include <ssl-tunnel/backend/peer.h>
#include <ssl-tunnel/backend/hashmap.h>
#include <ssl-tunnel/backend/trie.h>
#include <ssl-tunnel/backend/config.h>
#include <ssl-tunnel/backend/tunnel_recv.h>
#include <ssl-tunnel/backend/tunnel_send.h>

#include <stdio.h>
#include <unistd.h> // close
#include <sys/epoll.h>
#include <netinet/in.h> // struct sockaddr

typedef struct {
    memscope_t m;

    slice_t(peer_t) peers;
    hashmap_t index_lookup; // [uint32_t remote_index] -> peer_t *peer
    trie_t route_lookup;

    int poll_fd;
    inbound_queue_t recv_q;
    outbound_queue_t send_q;
} tunnel_t;

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
    deque_init((deque_any_t *) &t->recv_q, sizeof(inbound_packet_t), &t->m.alloc);
    deque_init((deque_any_t *) &t->send_q, sizeof(outbound_packet_t), &t->m.alloc);
}

static void tunnel_free(tunnel_t *t) {
    if (t->poll_fd >= 0) {
        close(t->poll_fd);
    }

    memscope_free(&t->m);
}

err_t tunnel_main(const config_t *cfg, int tun_fd, int socket_fd, int signal_fd) {
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

        hashmap_insert(&t.index_lookup, p->index,  &p);

        trie_insert(&t.route_lookup, p->cfg_entry->addr, p->cfg_entry->addr_prefix, &p);
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

    if (!ERROR_OK(err = fd_poll_add(t.poll_fd, signal_fd, FD_POLL_READ))) {
        goto cleanup;
    }

    struct epoll_event evs[2];

    if (optional_is_some(cfg->interface.listen_port)) {
        printf("server is listening on port %d\n", optional_unwrap(cfg->interface.listen_port));
        fflush(stdout);
    }

    bool running = true;
    while (running) {
        int fd_ready;
        err = fd_poll_wait(t.poll_fd, evs, 2, -1, &fd_ready);
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
            if (fd == socket_fd) {
                tunnel_handle_socket_read(&t, tun_fd, socket_fd);
                continue;
            }

            if (fd != signal_fd) {
                panicf("poll returned unknown fd: %d", fd);
            }

            running = false;
            break;
        }
    }
    printf("signal received, exiting...\n");

cleanup:
    tunnel_free(&t);
    return err;
}
