#pragma once

#include <ssl-tunnel/lib/err.h>

#include <stdint.h>

#define PROTO_MAX_MTU 1500

#define PROTO_HEADER_IP_MIN_LEN 20
#define PROTO_HEADER_UDP_LEN 8
#define PROTO_HEADER_TRANSPORT_LEN 20

typedef struct {
    uint32_t packet_type;
#define PROTO_PACKET_TYPE_TRANSPORT 3
    uint32_t remote_index;
    uint8_t nonce[12];
#define PROTO_TRANSPORT_MAX_DATA_LEN (PROTO_MAX_MTU - PROTO_HEADER_IP_MIN_LEN - PROTO_HEADER_UDP_LEN - PROTO_HEADER_TRANSPORT_LEN)
    uint8_t data[PROTO_TRANSPORT_MAX_DATA_LEN];
} proto_transport_t;

extern const err_t ERR_DATA_TOO_LARGE;

err_t proto_new_transport_packet(uint32_t remote_index, uint8_t nonce[12], const uint8_t *data, size_t data_len,
                                 proto_transport_t *out_packet, size_t *out_len);

typedef struct {
    uint8_t vhl;        /* version << 4 | header length >> 2 */
    uint8_t  tos;       /* type of service */
    uint16_t len;       /* total length */
    uint16_t id;        /* identification */
    uint16_t off;       /* fragment offset field */
    uint8_t  ttl;       /* time to live */
    uint8_t  p;         /* protocol */
    uint16_t sum;       /* checksum */
    uint32_t src, dst;  /* source and dest address */
#define PROTO_IP_MAX_DATA_LEN (PROTO_MAX_MTU - PROTO_HEADER_IP_MIN_LEN)
    uint8_t data[PROTO_IP_MAX_DATA_LEN];
} proto_ip_t;

#define PROTO_CIPHER_NONE 0
#define PROTO_CIPHER_NULL_SHA384 1
#define PROTO_CIPHER_AES_256_GCM 2
#define PROTO_CIPHER_CHACHA20_POLY1305 3