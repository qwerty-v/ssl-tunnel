#pragma once

#include <ssl-tunnel/backend/proto.h>
#include <ssl-tunnel/backend/peer.h>
#include <ssl-tunnel/backend/hashmap.h>

#include <ssl-tunnel/lib/deque.h>

#include <pthread.h>

typedef struct {
    peer_t *peer;

    size_t packet_len;
    proto_transport_t packet;
} inbound_packet_t;

typedef deque_t(inbound_packet_t) inbound_queue_t;

#define INBOUND_BUF_SIZE 128

typedef struct {
    inbound_queue_t *recv_q;
    pthread_mutex_t *mx;
    inbound_packet_t *in_q;
} inbound_t;

void io_udp_read(int socket_fd, inbound_t *in, const hashmap_t *index_lookup);

void io_tun_write(int tun_fd, inbound_t *in);
