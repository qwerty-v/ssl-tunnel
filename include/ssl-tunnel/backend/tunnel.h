#pragma once

#include <ssl-tunnel/backend/config.h>

#include <ssl-tunnel/lib/err.h>

err_t tunnel_main(const config_t *cfg, int tun_fd, int socket_fd, int signal_fd);