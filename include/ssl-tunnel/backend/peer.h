#pragma once

#include <ssl-tunnel/backend/config.h>

#include <netinet/in.h> // struct sockaddr, socklen_t
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    const config_peer_t *cfg_entry;

    uint32_t index;

    bool is_remote_addr_known;
    struct sockaddr_in remote_addr;
    socklen_t remote_addr_len;
} peer_t;
