#include <ssl-tunnel/alloc.h>

#include <stdlib.h>

void *allocator_call_calloc(alloc_t a, size_t nmemb, size_t size) {
    return (*a.calloc)(a.calloc, nmemb, size);
}

void *allocator_call_malloc(alloc_t a, size_t size) {
    return (*a.malloc)(a.malloc, size);
}

void *allocator_call_realloc(alloc_t a, void *ptr, size_t size) {
    return (*a.realloc)(a.realloc, ptr, size);
}

calloc_t _std_calloc = std_calloc;
malloc_t _std_malloc = std_malloc;
realloc_t _std_realloc = std_realloc;

const alloc_t std_alloc = {
        .calloc = &_std_calloc,
        .malloc = &_std_malloc,
        .realloc = &_std_realloc
};

void *std_calloc(void *closure, size_t nmemb, size_t size) {
    (void)closure;
    return calloc(nmemb, size);
}

void *std_malloc(void *closure, size_t size) {
    (void)closure;
    return malloc(size);
}

void *std_realloc(void *closure, void *ptr, size_t size) {
    (void)closure;
    return realloc(ptr, size);
}