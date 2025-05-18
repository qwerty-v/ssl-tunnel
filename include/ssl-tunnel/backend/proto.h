#pragma once

#include <stdint.h>

#define PROTO_MAX_MTU 1500

typedef struct {
    size_t len;
    uint8_t bytes[PROTO_MAX_MTU];
} proto_raw_t;

typedef struct {
    uint32_t packet_type;
    uint32_t session_id;
    uint32_t nonce;
    uint8_t data[PROTO_MAX_MTU - 40 - 12];
} proto_transport_t;