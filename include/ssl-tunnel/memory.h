#pragma once

#include <ssl-tunnel/errors.h>
#include <ssl-tunnel/arrays.h>

#include <stddef.h>

typedef struct {
    void *ptr;
    size_t size;
} object;

typedef struct {
    slice *allocated_objs;
} scope;

err_t mem_alloc_scope(scope **m);
void mem_destroy_scope(scope *m);

err_t scope_init(scope *m);
err_t scope_alloc(scope *m, void **dst, size_t size);
