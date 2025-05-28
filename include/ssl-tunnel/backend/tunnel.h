#pragma once

#include <ssl-tunnel/backend/config.h>

#include <ssl-tunnel/lib/err.h>

#include <stdatomic.h> // atomic_bool

err_t tunnel_event_loop(const config_t *cfg, int tun_fd, int socket_fd, const volatile atomic_bool *flag);