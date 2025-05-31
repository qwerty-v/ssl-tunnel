#pragma once

#include <ssl-tunnel/lib/alloc.h> // alloc_t
#include <ssl-tunnel/lib/slice.h> // slice_t

typedef struct {
    alloc_t alloc;
    slice_t(void *) ptrs;
} memscope_t;

void memscope_init(memscope_t *m);

void memscope_free(memscope_t *m);
