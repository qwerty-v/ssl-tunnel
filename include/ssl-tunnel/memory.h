#pragma once

#include <ssl-tunnel/errors.h>
#include <ssl-tunnel/arrays.h>
#include <ssl-tunnel/alloc.h>

// for the sake of testing, export the internal object
typedef struct {
    void *ptr;
    size_t size;
} _object_t;

typedef struct {
    slice_t objs;
} alloc_pool_t;

void alloc_pool_free(alloc_pool_t *p);

void alloc_pool_init(alloc_pool_t *p);
err_t alloc_pool_get_allocator(alloc_pool_t *p, alloc_t *out);
