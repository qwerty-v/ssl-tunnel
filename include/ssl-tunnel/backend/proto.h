#pragma once

#include <ssl-tunnel/lib/err.h>

#include <stdint.h>

#define PROTO_MAX_MTU 1500

#define PROTO_IP_MIN_HEADER_LEN 20
#define PROTO_TRANSPORT_HEADER_LEN 16

#define PROTO_MAX_DATA_SIZE (PROTO_MAX_MTU - PROTO_IP_MIN_HEADER_LEN - PROTO_TRANSPORT_HEADER_LEN)

#define PROTO_PACKET_TYPE_TRANSPORT 3

typedef struct {
    uint32_t packet_type;
    uint32_t remote_index;
    uint64_t nonce;
    uint8_t data[PROTO_MAX_DATA_SIZE];
} proto_transport_t;

extern const err_t ERR_DATA_TOO_LARGE;

err_t proto_new_transport_packet(uint32_t remote_index, uint64_t nonce, const uint8_t *data, size_t data_len,
                                 proto_transport_t *out_packet, size_t *out_len);