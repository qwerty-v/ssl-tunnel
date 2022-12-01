#ifndef SSL_TUNNEL_BUFFER_H
#define SSL_TUNNEL_BUFFER_H

#include <stddef.h>
#include <stdint.h>

struct buffer {
    uint8_t *buf;
    int capacity;
    int size;
};

struct buffer alloc_buf(size_t size);

void free_buf(struct buffer b);

#endif //SSL_TUNNEL_BUFFER_H
