#pragma once

#include <stddef.h> // size_t

typedef void *(*alloc_calloc_fn_t)(void *extra, size_t nmemb, size_t size);

typedef void *(*alloc_malloc_fn_t)(void *extra, size_t size);

typedef void *(*alloc_realloc_fn_t)(void *extra, void *ptr, size_t size);

typedef void (*alloc_free_fn_t)(void *extra, void *ptr);

typedef struct {
    alloc_calloc_fn_t calloc_fn;
    alloc_malloc_fn_t malloc_fn;
    alloc_realloc_fn_t realloc_fn;
    alloc_free_fn_t free_fn;

    void *extra;
} alloc_t;

extern const alloc_t alloc_std;

void *alloc_calloc(const alloc_t *a, size_t nmemb, size_t size);

void *alloc_malloc(const alloc_t *a, size_t size);

void *alloc_realloc(const alloc_t *a, void *ptr, size_t size);

void alloc_free(const alloc_t *a, void *ptr);
