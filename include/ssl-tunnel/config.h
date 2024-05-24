#pragma once

#include <ssl-tunnel/errors.h>

typedef struct {
    int server_port;
    int device_mtu;
    char *device_name;
} config_t;

err_t config_read(const char *cfg_path, config_t *cfg);