#pragma once

#include <ssl-tunnel/lib/err.h>

typedef struct {
    int server_port;
    int device_mtu;
    char *device_name;
} config_t;

err_t config_read(const char *cfg_path, memory_set_t *m, config_t *cfg);