#include <ssl-tunnel/lib/alloc.h>

#include <stdlib.h>

static void *std_calloc(void *extra, size_t nmemb, size_t size) {
    (void) extra;
    return calloc(nmemb, size);
}

static void *std_malloc(void *extra, size_t size) {
    (void) extra;
    return malloc(size);
}

static void *std_realloc(void *extra, void *ptr, size_t size) {
    (void) extra;
    return realloc(ptr, size);
}

static void std_free(void *extra, void *ptr) {
    (void) extra;
    free(ptr);
}

const alloc_t alloc_std = {
        .calloc_fn = &std_calloc,
        .malloc_fn = &std_malloc,
        .realloc_fn = &std_realloc,
        .free_fn = &std_free,

        .extra = NULL
};

void *alloc_calloc(const alloc_t *a, size_t nmemb, size_t size) {
    return a->calloc_fn(a->extra, nmemb, size);
}

void *alloc_malloc(const alloc_t *a, size_t size) {
    return a->malloc_fn(a->extra, size);
}

void *alloc_realloc(const alloc_t *a, void *ptr, size_t size) {
    return a->realloc_fn(a->extra, ptr, size);
}

void alloc_free(const alloc_t *a, void *ptr) {
    a->free_fn(a->extra, ptr);
}
