#include <ssl-tunnel/config.h>

err_t config_read(const char *cfg_path, config_t *cfg) {
    // fixme
    cfg->server_port = 1026;
    cfg->device_mtu = 1500;
    cfg->device_name = "tun0";

    return ERROR_OK;
}