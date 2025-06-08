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
#include <pthread.h>

#define CPU_COUNT 4

typedef struct {
    memscope_t m;
    memscope_t m2;

    slice_t(peer_t) peers;
    hashmap_t index_lookup; // [uint32_t remote_index] -> peer_t *peer
    trie_t route_lookup;

    inbound_queue_t recv_q;
    outbound_queue_t send_q;
    pthread_mutex_t recv_q_mx;
    pthread_mutex_t send_q_mx;

    int poll_fd;
    int tun_pending_fd;
    int socket_pending_fd;

    int self_index;
    int tun_fd;
    int socket_fd;
    const volatile atomic_bool *flag;
} tunnel_t;

static void *worker_main(void *p) {
    tunnel_t *t = p;

    inbound_packet_t *in_q = alloc_malloc(&alloc_std, INBOUND_BUF_SIZE * sizeof(inbound_packet_t));
    outbound_packet_t *out_q = alloc_malloc(&alloc_std, OUTBOUND_BUF_SIZE * sizeof(outbound_packet_t));

    struct epoll_event ev;
    while (atomic_load(t->flag)) {
        int fd_ready;
        err_t err = fd_poll_wait(t->poll_fd, &ev, 1, -1, &fd_ready);
        if (!ERROR_OK(err)) {
            panicf("error during fd_poll_wait(): %s", err.msg);
        }
        if (fd_ready != 1) {
            panicf("wrong fd_ready: %d", fd_ready);
        }

        int fd = ev.data.fd;
        if (fd == t->tun_fd) {
//            printf("tun_fd\n");
//            fflush(stdout);

            io_tun_read(t->tun_fd, &t->send_q, &t->send_q_mx, &t->route_lookup, out_q);

            uint64_t num = 1;
            write(t->socket_pending_fd, &num, sizeof(uint64_t));
            continue;
        }
        if (fd == t->socket_fd) {
//            printf("socket_fd\n");
//            fflush(stdout);

            io_udp_read(t->socket_fd, &t->recv_q, &t->recv_q_mx, &t->index_lookup, in_q);

            uint64_t num = 1;
            write(t->tun_pending_fd, &num, sizeof(uint64_t));
            continue;
        }
        if (fd == t->socket_pending_fd) {
//            printf("socket_pending_fd\n");
//            fflush(stdout);

            uint64_t num;
            while (read(t->socket_pending_fd, &num, sizeof(uint64_t)) > 0);

            io_udp_write(t->socket_fd, &t->send_q, &t->send_q_mx, t->self_index, out_q);
            continue;
        }
        if (fd == t->tun_pending_fd) {
//            printf("tun_pending_fd\n");
//            fflush(stdout);

            uint64_t num;
            while (read(t->tun_pending_fd, &num, sizeof(uint64_t)) > 0);

            io_tun_write(t->tun_fd, &t->recv_q, &t->recv_q_mx, in_q);
            continue;
        }

        panicf("unknown fd after fd_poll_wait()");
    }

    alloc_free(&alloc_std, in_q);
    alloc_free(&alloc_std, out_q);

    return NULL;
}

static void tunnel_init(tunnel_t *t) {
    memset(t, 0, sizeof(tunnel_t));

    memscope_init(&t->m);
    memscope_init(&t->m2);

    slice_init((slice_any_t *) &t->peers, sizeof(peer_t), &t->m.alloc);
    hashmap_init(&t->index_lookup, &t->m.alloc);
    trie_init(&t->route_lookup, &t->m.alloc);

    deque_init((deque_any_t *) &t->recv_q, sizeof(inbound_packet_t), &t->m.alloc);
    deque_init((deque_any_t *) &t->send_q, sizeof(outbound_packet_t), &t->m2.alloc);

    pthread_mutex_init(&t->recv_q_mx, NULL);
    pthread_mutex_init(&t->send_q_mx, NULL);

    t->poll_fd = -1;
    t->tun_pending_fd = -1;
    t->socket_pending_fd = -1;
}

static void tunnel_free(tunnel_t *t) {
    if (t->poll_fd >= 0) {
        close(t->poll_fd);
    }
    if (t->tun_pending_fd >= 0) {
        close(t->tun_pending_fd);
    }
    if (t->socket_pending_fd >= 0) {
        close(t->socket_pending_fd);
    }

    pthread_mutex_destroy(&t->recv_q_mx);
    pthread_mutex_destroy(&t->send_q_mx);

    memscope_free(&t->m);
    memscope_free(&t->m2);
}

err_t tunnel_main(const config_t *cfg, int tun_fd, int socket_fd, int signal_fd) {
    tunnel_t t;
    tunnel_init(&t);

    t.self_index = cfg->interface.index;
    t.tun_fd = tun_fd;
    t.socket_fd = socket_fd;
    t.flag = flag;

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

    if (!ERROR_OK(err = fd_eventfd(&t.tun_pending_fd))) {
        goto cleanup;
    }

    if (!ERROR_OK(err = fd_poll_add(t.poll_fd, t.tun_pending_fd, FD_POLL_READ))) {
        goto cleanup;
    }

    if (!ERROR_OK(err = fd_eventfd(&t.socket_pending_fd))) {
        goto cleanup;
    }

    if (!ERROR_OK(err = fd_poll_add(t.poll_fd, t.socket_pending_fd, FD_POLL_READ))) {
        goto cleanup;
    }

    if (optional_is_some(cfg->interface.listen_port)) {
        printf("server is listening on port %d\n", optional_unwrap(cfg->interface.listen_port));
        fflush(stdout);
    }

    deque_resize((deque_any_t *) &t.recv_q, 128);
    deque_resize((deque_any_t *) &t.send_q, 128);

    pthread_t workers[CPU_COUNT];
    for (int i = 0; i < CPU_COUNT; i++) {
        pthread_create(&workers[i], NULL, worker_main, &t);
    }

    for (int i = 0; i < CPU_COUNT; i++) {
        pthread_join(workers[i], NULL);
    }

cleanup:
    tunnel_free(&t);
    return err;
}
