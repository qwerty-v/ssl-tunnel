#ifndef SSL_TUNNEL_PROTO_H
#define SSL_TUNNEL_PROTO_H

#include "target.h"
#include "buffer.h"

#ifdef TARGET_APPLE
#include "netinet/ip.h"

typedef struct ip proto_iphdr;

#elifdef TARGER_LINUX

#error "not implemented"

#else
#error "not implemented"
#endif

uint8_t proto_ip_get_version(struct buffer b);

#endif //SSL_TUNNEL_PROTO_H
