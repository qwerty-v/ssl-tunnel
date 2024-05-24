#pragma once

#include <stddef.h>

typedef void *(*calloc_t)(void *closure, size_t nmemb, size_t size);
typedef void *(*malloc_t)(void *closure, size_t size);
typedef void *(*realloc_t)(void *closure, void *ptr, size_t size);

typedef struct {
    calloc_t *calloc;
    malloc_t *malloc;
    realloc_t *realloc;
} alloc_t;

void *allocator_call_calloc(alloc_t a, size_t nmemb, size_t size);
void *allocator_call_malloc(alloc_t a, size_t size);
void *allocator_call_realloc(alloc_t a, void *ptr, size_t size);

void *std_calloc(void *closure, size_t nmemb, size_t size);
void *std_malloc(void *closure, size_t size);
void *std_realloc(void *closure, void *ptr, size_t size);

extern const alloc_t std_alloc;
