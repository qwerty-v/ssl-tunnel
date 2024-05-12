#pragma once

#include <ssl-tunnel/errors.h>

#include <stddef.h>

typedef struct {
    int cap;
    int len;
    void *elements;
    size_t element_size;
} slice;

err_t array_alloc_slice(slice **s);
void array_destroy_slice(slice *s);

void slice_init(slice *s, size_t element_size);
err_t slice_append(slice *s, void *element);
