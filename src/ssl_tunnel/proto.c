#include "buffer.h"

uint8_t proto_ip_get_version(struct buffer b) {
    return (b.buf[0] >> 4) & 0x0F;
}