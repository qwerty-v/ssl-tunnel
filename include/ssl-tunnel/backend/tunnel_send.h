#pragma once

#include <ssl-tunnel/backend/peer.h>
#include <ssl-tunnel/backend/trie.h>
#include <ssl-tunnel/backend/proto.h>

#include <ssl-tunnel/lib/deque.h>

typedef struct {
    peer_t *peer;

    size_t packet_len;
    proto_ip_t packet;
} outbound_packet_t;

typedef deque_t(outbound_packet_t) outbound_queue_t;

void io_tun_read(int tun_fd, outbound_queue_t *send_q, const trie_t *route_lookup);

void io_udp_write(int socket_fd, outbound_queue_t *send_q, uint32_t self_index);
