#include "buffer.h"

#include <stdlib.h>

struct buffer alloc_buf(const size_t size) {
    struct buffer b;

    b.capacity = (int) size;
    b.size = 0;
    b.buf = malloc(sizeof(char) * size);

    return b;
}

void free_buf(struct buffer b) {
    if (b.buf) {
        free(b.buf);
    }
}