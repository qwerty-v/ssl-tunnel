#include <ssl-tunnel/lib/memory.h>
#include <ssl-tunnel/lib/alloc.h>
#include <ssl-tunnel/lib/err.h>
#include <ssl-tunnel/lib/slice.h>

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

static void *memory_set_calloc_fn(void *extra, size_t nmemb, size_t size);

static void *memory_set_malloc_fn(void *extra, size_t size);

static void *memory_set_realloc_fn(void *extra, void *ptr, size_t size);

static void memory_set_free_fn(void *extra, void *ptr);

void memory_set_init(memory_set_t *m) {
    memset(m, 0, sizeof(memory_set_t));

    slice_init((slice_any_t *) &m->ptrs, sizeof(void *), &alloc_std);

    m->alloc = (alloc_t) {
            .calloc_fn = &memory_set_calloc_fn,
            .malloc_fn = &memory_set_malloc_fn,
            .realloc_fn = &memory_set_realloc_fn,
            .free_fn = &memory_set_free_fn,

            .extra = m
    };
}

static void memory_set_append(memory_set_t *m, void *ptr) {
    slice_append((slice_any_t *) &m->ptrs, ptr);
}

static void *memory_set_calloc_fn(void *extra, size_t nmemb, size_t size) {
    void *ptr = calloc(nmemb, size);
    if (!ptr) {
        panicf("out of memory");
    }

    memory_set_t *m = extra;

    memory_set_append(m, ptr);

    return ptr;
}

static void *memory_set_malloc_fn(void *extra, size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        panicf("out of memory");
    }

    memory_set_t *m = extra;

    memory_set_append(m, ptr);

    return ptr;
}

static void *memory_set_realloc_fn(void *extra, void *ptr, size_t size) {
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr) {
        panicf("out of memory");
    }

    memory_set_t *m = extra;

    int to_update = -1;
    for (int i = 0; i < (int) m->ptrs.len; i++) {
        if (m->ptrs.array[i] == ptr) {
            to_update = i;
            break;
        }
    }
    if (to_update == -1) {
        panicf("call to realloc without prior allocation");
    }

    m->ptrs.array[to_update] = new_ptr;

    return new_ptr;
}

static void memory_set_free_fn(void *extra, void *ptr) {
    free(ptr);

    memory_set_t *m = extra;

    int to_remove = -1;
    for (int i = 0; i < (int) m->ptrs.len; i++) {
        if (m->ptrs.array[i] == ptr) {
            to_remove = i;
            break;
        }
    }
    if (to_remove == -1) {
        panicf("call to free without prior allocation");
    }

    err_t err = slice_remove((slice_any_t *) &m->ptrs, to_remove);
    if (!ERROR_OK(err)) {
        panicf("error removing allocated memory pointer: %s", err.msg);
    }
}

void memory_set_free(memory_set_t *m) {
    for (int i = 0; i < (int) m->ptrs.len; i++) {
        free(m->ptrs.array[i]);
    }

    // alloc_std won't free itself
    if (m->ptrs.cap > 0) {
        free(m->ptrs.array);
    }
}
