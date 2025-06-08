#pragma once

#include <ssl-tunnel/backend/peer.h>
#include <ssl-tunnel/backend/trie.h>
#include <ssl-tunnel/backend/proto.h>

#include <ssl-tunnel/lib/deque.h>

#include <pthread.h>

typedef struct {
    peer_t *peer;

    size_t packet_len;
    proto_ip_t packet;
} outbound_packet_t;

typedef deque_t(outbound_packet_t) outbound_queue_t;

#define OUTBOUND_BUF_SIZE 128

void io_tun_read(int tun_fd, outbound_queue_t *send_q, pthread_mutex_t *mx, const trie_t *route_lookup, outbound_packet_t *out_q);

void io_udp_write(int socket_fd, outbound_queue_t *send_q, pthread_mutex_t *mx, uint32_t self_index, outbound_packet_t *out_q);
