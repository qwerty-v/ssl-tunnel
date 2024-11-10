#include <ssl-tunnel/lib/alloc.h>

#include <stdlib.h>

void *alloc_calloc(const alloc_t *a, size_t nmemb, size_t size) {
    return (**a->_calloc)(a->_calloc, nmemb, size);
}

void *alloc_malloc(const alloc_t *a, size_t size) {
    return (**a->_malloc)(a->_malloc, size);
}

void *alloc_realloc(const alloc_t *a, void *ptr, size_t size) {
    return (**a->_realloc)(a->_realloc, ptr, size);
}

void alloc_free(const alloc_t *a, void *ptr) {
    (**a->_free)(a->_free, ptr);
}

void *std_calloc(void *closure, size_t nmemb, size_t size) {
    (void) closure;
    return calloc(nmemb, size);
}

void *std_malloc(void *closure, size_t size) {
    (void) closure;
    return malloc(size);
}

void *std_realloc(void *closure, void *ptr, size_t size) {
    (void) closure;
    return realloc(ptr, size);
}

void std_free(void *closure, void *ptr) {
    (void) closure;
    free(ptr);
}

const alloc_t *alloc_std = &(alloc_t) {
        ._calloc = &(alloc_calloc_fn_t) {&std_calloc},
        ._malloc = &(alloc_malloc_fn_t) {&std_malloc},
        ._realloc = &(alloc_realloc_fn_t) {&std_realloc},
        ._free = &(alloc_free_fn_t) {&std_free}
};
