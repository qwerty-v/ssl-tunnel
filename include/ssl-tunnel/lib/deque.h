#pragma once

#include <ssl-tunnel/lib/err.h>
#include <ssl-tunnel/lib/alloc.h>

#include <stdint.h>

typedef struct {
    size_t cap;
    size_t len;
    void *array;
    int front;
    int back;
    size_t element_size;
    const alloc_t *allocator;
} deque_t;

extern const err_t ERR_DEQUE_CAP_TOO_LOW;
extern const err_t ERR_DEQUE_EMPTY;

void deque_init(deque_t *d, size_t element_size, const alloc_t *a);

err_t deque_resize(deque_t *d, size_t new_cap);

void deque_push_back(deque_t *d, const void *element);

err_t deque_pop_back(deque_t *d);

void deque_push_front(deque_t *d, const void *element);

err_t deque_pop_front(deque_t *d);
