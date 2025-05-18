#pragma once

#include <ssl-tunnel/lib/slice.h> // slice_t
#include <ssl-tunnel/lib/alloc.h> // alloc_t

typedef struct {
    slice_t(void *) ptrs;
    alloc_t alloc;
} memory_set_t;

void memory_set_init(memory_set_t *m);

void memory_set_free(memory_set_t *m);
