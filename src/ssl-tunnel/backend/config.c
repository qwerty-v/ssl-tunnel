#include <ssl-tunnel/lib/optional.h>
#include <ssl-tunnel/lib/memory.h>
#include <ssl-tunnel/lib/err.h>

#include <ssl-tunnel/backend/config.h>

err_t config_read(const char *path, config_t *out_cfg) {
    // fixme
    out_cfg->interface.name = "tun0";
//    out_cfg->interface_cfg.addr = "10.8.0.1/24";
    out_cfg->interface.listen_port.present = true;
    out_cfg->interface.listen_port.v = 1026;

//    peer.pub_key = "123";
//    peer.remote = "example.com";
//    peer.addr = "10.8.0.1";

    return ENULL;
}