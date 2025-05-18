#pragma once

#include <ssl-tunnel/lib/optional.h>
#include <ssl-tunnel/lib/hashmap.h>
#include <ssl-tunnel/lib/slice.h>
#include <ssl-tunnel/lib/err.h>

#include <stdint.h>

typedef struct {
    uint8_t bytes[32];
} config_preshared_key_t;

typedef struct {
    const char *name;
    optional_t(int) listen_port;
} config_interface_t;

typedef struct {
    optional_t(uint32_t) remote;
    uint32_t addr;
    optional_t(config_preshared_key_t) preshared_key;
} config_peer_t;

typedef struct {
    config_interface_t interface;
    slice_t(config_peer_t) peers;

    hashmap_t remote_to_peer;
    hashmap_t addr_to_peer;
} config_t;

err_t config_read(const char *path, config_t *out_cfg);
