#include <ssl-tunnel/lib/memory.h>
#include <ssl-tunnel/lib/alloc.h>
#include <ssl-tunnel/lib/errors.h>
#include <ssl-tunnel/lib/arrays.h>

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

struct _memory_set_closure_s {
    void *fn;
    memory_set_t *m;
};

typedef struct {
    void *memory_ptr;
} _memory_set_element_t;

static void *_memory_set_calloc(void *closure, size_t nmemb, size_t size);

static void *_memory_set_malloc(void *closure, size_t size);

static void *_memory_set_realloc(void *closure, void *ptr, size_t size);

static err_t _memory_set_append(memory_set_t *m, void *memory_ptr);

void memory_set_init(memory_set_t *m) {
    memset(m, 0, sizeof(memory_set_t));

    slice_init(&m->allocations, sizeof(_memory_set_element_t), alloc_std);

    m->_closures = malloc(3 * sizeof(_memory_set_closure_t));
    if (!m->_closures) {
        panicf("out of memory");
    }

    m->_closures[0].fn = &_memory_set_calloc;
    m->_closures[0].m = m;
    m->_closures[1].fn = &_memory_set_malloc;
    m->_closures[1].m = m;
    m->_closures[2].fn = &_memory_set_realloc;
    m->_closures[2].m = m;

    m->allocator = (alloc_t) {
            ._calloc = (alloc_calloc_fn_t * )((uint8_t * ) & m->_closures[0] + offsetof(_memory_set_closure_t, fn)),
            ._malloc = (alloc_malloc_fn_t * )((uint8_t * ) & m->_closures[1] + offsetof(_memory_set_closure_t, fn)),
            ._realloc = (alloc_realloc_fn_t * )((uint8_t * ) & m->_closures[2] + offsetof(_memory_set_closure_t, fn))
    };
}

static void *_memory_set_calloc(void *closure, size_t nmemb, size_t size) {
    void *memory_ptr = calloc(nmemb, size);
    if (!memory_ptr) {
        panicf("out of memory");
    }

    _memory_set_closure_t *c = closure;

    err_t err = _memory_set_append(c->m, memory_ptr);
    if (!ERROR_OK(err)) {
        panicf("could not append allocated memory");
    }

    return memory_ptr;
}

static void *_memory_set_malloc(void *closure, size_t size) {
    void *memory_ptr = malloc(size);
    if (!memory_ptr) {
        panicf("out of memory");
    }

    _memory_set_closure_t *c = closure;

    err_t err = _memory_set_append(c->m, memory_ptr);
    if (!ERROR_OK(err)) {
        panicf("could not append allocated memory");
    }

    return memory_ptr;
}

static void *_memory_set_realloc(void *closure, void *ptr, size_t size) {
    void *memory_ptr = realloc(ptr, size);
    if (!memory_ptr) {
        panicf("out of memory");
    }

    _memory_set_closure_t *c = closure;
    _memory_set_element_t *arr = c->m->allocations.array;

    int to_update = -1;
    for (int i = 0; i < (int) c->m->allocations.len; i++) {
        if (arr[i].memory_ptr == memory_ptr) {
            to_update = i;
            break;
        }
    }
    if (to_update == -1) {
        panicf("call to realloc without prior allocation");
    }

    arr[to_update].memory_ptr = memory_ptr;

    return memory_ptr;
}

void memory_set_free(memory_set_t *m) {
    _memory_set_element_t *arr = m->allocations.array;
    for (int i = 0; i < (int) m->allocations.len; i++) {
        free(arr[i].memory_ptr);
    }

    if (m->allocations.cap > 0) {
        free(m->allocations.array);
    }

    free(m->_closures);
}

static err_t _memory_set_append(memory_set_t *m, void *memory_ptr) {
    return slice_append(&m->allocations, &(_memory_set_element_t) {
            .memory_ptr = memory_ptr,
    });
}

