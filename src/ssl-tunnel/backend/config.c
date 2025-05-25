#include <ssl-tunnel/lib/optional.h>
#include <ssl-tunnel/lib/memscope.h>
#include <ssl-tunnel/lib/err.h>

#include <ssl-tunnel/backend/config.h>

void config_init(config_t *cfg, const alloc_t *alloc) {
    memset(cfg, 0, sizeof(config_t));

    cfg->alloc = alloc;

    slice_init((slice_any_t *) &cfg->peers, sizeof(config_peer_t), alloc);
}

err_t config_read(const char *path, config_t *out_cfg) {
    // fixme
    (void) path;

    out_cfg->interface.index = 1565313305;
    out_cfg->interface.name = "tun0";
    out_cfg->interface.listen_port.v = 1026;
    out_cfg->interface.listen_port.present = true;

    config_peer_t *peer = alloc_calloc(out_cfg->alloc, 1, sizeof(config_peer_t));

    peer->index = 2736684749;
    peer->remote.present = false;
    peer->addr = 168296450; // 10.8.0.2
    peer->preshared_key.present = false;

    slice_append((slice_any_t *) &out_cfg->peers, peer);

    return ENULL;
}