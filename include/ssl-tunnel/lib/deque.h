#pragma once

#include <ssl-tunnel/lib/err.h>
#include <ssl-tunnel/lib/alloc.h>

#include <stdint.h>

#define deque_t(T)          \
struct {                    \
    size_t cap;             \
    size_t len;             \
    size_t element_size;    \
    const alloc_t *alloc;   \
    size_t front;           \
    size_t back;            \
    T *array;               \
}

typedef deque_t(void) deque_any_t;

extern const err_t ERR_DEQUE_CAP_TOO_LOW;
extern const err_t ERR_DEQUE_EMPTY;

void deque_init(deque_any_t *d, size_t element_size, const alloc_t *a);

err_t deque_resize(deque_any_t *d, size_t new_cap);

void deque_push_back(deque_any_t *d, const void *element);

void deque_push_front(deque_any_t *d, const void *element);

err_t deque_back(deque_any_t *d, void *out_dst);

err_t deque_pop_back(deque_any_t *d);

err_t deque_front(deque_any_t *d, void *out_dst);

err_t deque_pop_front(deque_any_t *d);
