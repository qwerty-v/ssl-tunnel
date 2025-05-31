#include <ssl-tunnel/lib/optional.h>
#include <ssl-tunnel/lib/memscope.h>
#include <ssl-tunnel/lib/err.h>
#include <ssl-tunnel/lib/alloc.h>

#include <ssl-tunnel/backend/config.h>

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

static void trim_left(char **s) {
    while (**s && isspace((unsigned char)**s)) {
        (*s)++;
    }
}

static void trim_right(char *s) {
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[len - 1] = '\0';
        len--;
    }
}

static void trim(char **s) {
    trim_left(s);
    trim_right(*s);
}

static char *alloc_strdup(const alloc_t *alloc, const char *src) {
    size_t len = strlen(src);
    char *dst = alloc_malloc(alloc, len + 1);

    memcpy(dst, src, len);
    dst[len] = '\0';

    return dst;
}

static void parse_addr_prefix(const char *s, uint32_t *addr, uint8_t *prefix) {
    // s = "10.8.0.2/24"
    char ip[32] = {0};
    int pfx = 0;
    if (sscanf(s, "%31[^/]/%d", ip, &pfx) != 2) {
        panicf("invalid address");
    }

    *prefix = (uint8_t) pfx;

    struct in_addr ip_addr;
    if (!inet_aton(ip, &ip_addr)) {
        panicf("invalid address");
    }
    *addr = ntohl(ip_addr.s_addr);
}

static void parse_remote(const char *s, struct sockaddr_in *addr) {
    char *colon = strchr(s, ':');
    if (!colon) {
        panicf("no port specified");
    }
    *colon = 0;

    addr->sin_family = AF_INET;
    if (inet_aton(s, &addr->sin_addr) != 1) {
        panicf("invalid peer remote: %s", s);
    }

    char *port = colon + 1;
    addr->sin_port = htons(atoi(port));
}

void config_init(config_t *cfg, const alloc_t *alloc) {
    memset(cfg, 0, sizeof(config_t));

    cfg->alloc = alloc;

    slice_init((slice_any_t *) &cfg->peers, sizeof(config_peer_t), alloc);
}

err_t config_read(const char *path, config_t *out_cfg) {
    FILE *f = fopen(path, "r");
    if (!f) {
        return err_errno();
    }

    char buf[512];
    int section = 0; // 0=none, 1=interface, 2=peers
    int in_peer = 0;

    config_peer_t cur_peer;

    while (fgets(buf, sizeof(buf), f)) {
        char *line = buf;

        trim(&line);
        if (strlen(line) == 0) continue;

        if (strcmp(line, "interface:") == 0) {
            section = 1;
            in_peer = 0;
            continue;
        }
        if (strcmp(line, "peers:") == 0) {
            section = 2;
            in_peer = 0;
            continue;
        }

        // peer beginning?
        if (section == 2 && line[0] == '-' && line[1] == ' ') {
            if (in_peer) {
                // append previous peer
                slice_append((slice_any_t *) &out_cfg->peers, &cur_peer);
            }

            memset(&cur_peer, 0, sizeof(config_peer_t));
            in_peer = 1;

            // skip "-"
            line++;
            trim_left(&line);
        }

        // key: value
        char *colon = strchr(line, ':');
        if (!colon) continue;
        *colon = 0;

        char *key = line;
        trim_right(key);

        char *val = colon + 1;
        trim_left(&val);

        // interface?
        if (section == 1) {
            if (strcmp(key, "index") == 0) {
                out_cfg->interface.index = (uint32_t)atoi(val);
                continue;
            }
            if (strcmp(key, "name") == 0) {
                out_cfg->interface.name = alloc_strdup(out_cfg->alloc, val);
                continue;
            }
            if (strcmp(key, "listen_port") == 0) {
                out_cfg->interface.listen_port.present = true;
                out_cfg->interface.listen_port.v = atoi(val);
                continue;
            }

            panicf("unknown interface key: %s", key);
        }

        // peer?
        if (section == 2 && in_peer) {
            if (strcmp(key, "index") == 0) {
                cur_peer.index = (uint32_t) atoi(val);
                continue;
            }
            if (strcmp(key, "remote") == 0) {
                cur_peer.remote.present = true;
                parse_remote(val, &cur_peer.remote.v);
                continue;
            }
            if (strcmp(key, "addr") == 0) {
                parse_addr_prefix(val, &cur_peer.addr, &cur_peer.addr_prefix);
                continue;
            }
            if (strcmp(key, "preshared_key") == 0) {
                // fixme
                size_t len = strlen(val);
                if (len > 32) len = 32;

                cur_peer.preshared_key.present = true;
                memcpy(cur_peer.preshared_key.v, val, len);
                continue;
            }

            panicf("unknown peer key: %s", key);
        }
    }
    fclose(f);

    // left over peer?
    if (in_peer) {
        slice_append((slice_any_t *) &out_cfg->peers, &cur_peer);
    }

    return ENULL;
}