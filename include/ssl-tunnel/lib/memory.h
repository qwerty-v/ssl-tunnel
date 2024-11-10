#pragma once

#include <ssl-tunnel/lib/slice.h> // slice_t
#include <ssl-tunnel/lib/alloc.h> // alloc_t

typedef struct _memory_set_closure_s _memory_set_closure_t;

typedef struct {
    slice_t allocations;
    alloc_t allocator;

    _memory_set_closure_t *_closures;
} memory_set_t;

void memory_set_init(memory_set_t *m);

void memory_set_free(memory_set_t *m);
