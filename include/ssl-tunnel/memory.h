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
} mem_scope;

err_t mem_alloc_mem_scope(mem_scope **m);
void mem_destroy_mem_scope(mem_scope *m);

err_t mem_scope_init(mem_scope *m);
err_t mem_scope_alloc(mem_scope *m, void **dst, size_t size);
