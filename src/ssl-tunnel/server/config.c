#include <ssl-tunnel/lib/memory.h>
#include <ssl-tunnel/lib/errors.h>
#include <ssl-tunnel/server/config.h>

err_t config_read(const char *cfg_path, memory_set_t *m, config_t *cfg) {
    // fixme
    cfg->server_port = 1026;
    cfg->device_mtu = 1500;
    cfg->device_name = "tun0";

    return ENULL;
}