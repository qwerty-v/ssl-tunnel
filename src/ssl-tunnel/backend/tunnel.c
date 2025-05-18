#include <ssl-tunnel/lib/memory.h>
#include <ssl-tunnel/lib/err.h>
#include <ssl-tunnel/lib/fd.h>
#include <ssl-tunnel/lib/deque.h>
#include <ssl-tunnel/lib/slice.h>
#include <ssl-tunnel/lib/hashmap.h>

#include <ssl-tunnel/backend/proto.h>
#include <ssl-tunnel/backend/config.h>

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdatomic.h> // atomic_bool, atomic_load
#include <sys/epoll.h>
#include <netinet/in.h>

//const err_t ERR_PEER_NOT_FOUND = {
//        .msg = "could not find peer"
//};

//typedef struct {
//    uint32_t session_id;
//    peer_t *peer;
//} session_t;

typedef struct {
    proto_raw_t packet;

    struct sockaddr remote_addr;
    socklen_t remote_addr_len;
} queued_packet_t;

typedef deque_t(queued_packet_t) packet_queue_t;

typedef struct {
    memory_set_t m;

    int poll_fd;

//    hashmap_t sessions; // [uint32_t session_id] -> session_t session

    packet_queue_t recv_q;
    packet_queue_t send_q;
} tunnel_t;

//static err_t _server_ifconfig_device_up(const config_t *cfg) {
//    char *fmt = "ifconfig %s 10.8.0.1 mtu %d netmask 255.255.255.0 up";
//
//    char *result_cmd = malloc(snprintf(0, 0, fmt, cfg->device_name, cfg->device_mtu) + 1);
//    sprintf(result_cmd, fmt, cfg->device_name, cfg->device_mtu);
//
//    printf("%s\n", result_cmd);
//    int status = system(result_cmd);
//    free(result_cmd);
//
//    if (status != 0) {
//        return ERR_SERVER_IFCONFIG_FAILED;
//    }
//
//    return ENULL;
//}

void io_udp_read(int socket_fd, packet_queue_t *recv_q) {
    // socket -> recv_q
    while (1) {
        queued_packet_t p;

        ssize_t n = recvfrom(socket_fd, p.packet.bytes, PROTO_MAX_MTU, 0, &p.remote_addr, &p.remote_addr_len);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            panicf("error calling recvfrom (errno %d)", errno);
        }
        p.packet.len = n;

        deque_push_back((deque_any_t *) recv_q, &p);
    }
}

void io_tun_write(int tun_fd, packet_queue_t *recv_q) {
    // recv_q -> tun
    while (recv_q->len > 0) {
        queued_packet_t p;
        err_t err = deque_front((deque_any_t *) recv_q, &p);
        if (!ERROR_OK(err)) {
            panicf("error reading front of the recv_q queue: %s", err.msg);
        }

        ssize_t n = write(tun_fd, p.packet.bytes, p.packet.len);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            panicf("error calling write (errno %d)", errno);
        }
        if ((size_t) n != p.packet.len) {
            panicf("unable to write whole buffer into device");
        }

        if (!ERROR_OK(err = deque_pop_front((deque_any_t *) recv_q))) {
            panicf("error removing queue element");
        }
    }
}

void io_tun_read(int tun_fd, packet_queue_t *send_q) {
    // tun -> send_q
    while (1) {
        queued_packet_t p;
        ssize_t n = read(tun_fd, p.packet.bytes, PROTO_MAX_MTU);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            panicf("error calling read (errno %d)", errno);
        }
        p.packet.len = n;

        deque_push_back((deque_any_t *) send_q, &p);
    }
}

void io_udp_write(int socket_fd, packet_queue_t *send_q) {
    // send_q -> udp
    while (send_q->len > 0) {
        queued_packet_t p;

        err_t err = deque_front((deque_any_t *) send_q, &p);
        if (!ERROR_OK(err)) {
            panicf("error reading front of the send_q queue: %s", err.msg);
        }

        ssize_t n = sendto(socket_fd, p.packet.bytes, p.packet.len, 0, &p.remote_addr, p.remote_addr_len);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            panicf("error calling sendto (errno %d)", errno);
        }
        if ((size_t) n != p.packet.len) {
            panicf("could not send whole buffer to client");
        }

        if (!ERROR_OK(err = deque_pop_front((deque_any_t *) send_q))) {
            panicf("error removing queue element");
        }
    }
}

static void tunnel_handle_socket_read(tunnel_t *t, int tun_fd, int socket_fd) {
    io_udp_read(socket_fd, &t->recv_q);

    io_tun_write(tun_fd, &t->recv_q);

    if (t->recv_q.len > 0) {
        fprintf(stderr, "warn: device busy\n");
    }
}

static void tunnel_handle_tun_read(tunnel_t *t, int tun_fd, int socket_fd) {
    io_tun_read(tun_fd, &t->send_q);

    io_udp_write(socket_fd, &t->send_q);

    if (t->send_q.len > 0) {
        fprintf(stderr, "warn: socket busy\n");
    }
}

static void tunnel_init(tunnel_t *t) {
    memory_set_init(&t->m);

    t->poll_fd = -1;

    deque_init((deque_any_t *) &t->recv_q, sizeof(proto_transport_t), &t->m.alloc);
    deque_init((deque_any_t *) &t->send_q, sizeof(proto_transport_t), &t->m.alloc);
}

static void tunnel_free(tunnel_t *t) {
    if (t->poll_fd >= 0) {
        close(t->poll_fd);
    }

    memory_set_free(&t->m);
}

err_t tunnel_event_loop(const config_t *cfg, int tun_fd, int socket_fd, const volatile atomic_bool *flag) {
    tunnel_t t;
    tunnel_init(&t);

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
                tunnel_handle_tun_read(&t, tun_fd, socket_fd);
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