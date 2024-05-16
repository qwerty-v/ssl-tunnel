#pragma once

#include <ssl-tunnel/errors.h>
#include <ssl-tunnel/alloc.h>
#include <ssl-tunnel/optional.h>

#include <stddef.h>

typedef struct {
    int cap;
    int len;
    void *array;
    size_t element_size;
    alloc_t allocator;
} slice_t;

typedef optional_t(alloc_t) optional_alloc_t;

extern const err_t ERROR_NEW_CAP_TOO_LOW;
extern const err_t ERROR_INDEX_OUT_OF_BOUNDS;

void slice_init(slice_t *s, size_t element_size, optional_alloc_t allocator);

err_t slice_resize(slice_t *s, int new_cap);
err_t slice_append(slice_t *s, const void *element);
err_t slice_remove(slice_t *s, int i);
err_t slice_ith(slice_t *s, int i, void *out);
