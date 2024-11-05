#pragma once

#include <stddef.h> // size_t

typedef void *(*alloc_calloc_fn_t)(void *closure, size_t nmemb, size_t size);

typedef void *(*alloc_malloc_fn_t)(void *closure, size_t size);

typedef void *(*alloc_realloc_fn_t)(void *closure, void *ptr, size_t size);

typedef struct {
    alloc_calloc_fn_t *_calloc;
    alloc_malloc_fn_t *_malloc;
    alloc_realloc_fn_t *_realloc;
} alloc_t;

void *alloc_calloc(const alloc_t *a, size_t nmemb, size_t size);

void *alloc_malloc(const alloc_t *a, size_t size);

void *alloc_realloc(const alloc_t *a, void *ptr, size_t size);

extern const alloc_t *alloc_std;
