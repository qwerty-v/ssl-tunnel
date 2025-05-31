#pragma once

#include <ssl-tunnel/lib/optional.h>
#include <ssl-tunnel/lib/slice.h>
#include <ssl-tunnel/lib/err.h>

#include <stdint.h>
#include <netinet/in.h> // struct sockaddr_in

typedef struct {
    uint32_t index;
    const char *name;
    optional_t(int) listen_port;
} config_interface_t;

typedef struct {
    uint32_t index;
    optional_t(struct sockaddr_in) remote;
    uint32_t addr;
    uint8_t addr_prefix;
    optional_arr_t(uint8_t, 32) preshared_key;
} config_peer_t;

typedef struct {
    const alloc_t *alloc;

    config_interface_t interface;
    slice_t(config_peer_t) peers;
} config_t;

void config_init(config_t *cfg, const alloc_t *alloc);

err_t config_read(const char *path, config_t *out_cfg);
