#pragma once

#include <ssl-tunnel/lib/err.h> // err_t
#include <ssl-tunnel/lib/alloc.h> // alloc_t

#include <stddef.h> // size_t

typedef struct {
    size_t cap;
    size_t len;
    void *array;
    size_t element_size;
    const alloc_t *allocator;
} slice_t;

extern const err_t ERR_SLICE_CAP_TOO_LOW;
extern const err_t ERR_SLICE_INDEX_OUT_OF_BOUNDS;

void slice_init(slice_t *s, size_t element_size, const alloc_t *a);

err_t slice_resize(slice_t *s, size_t new_cap);

void slice_append(slice_t *s, const void *element);

err_t slice_remove(slice_t *s, int i);

err_t slice_ith(const slice_t *s, int i, void *out);
