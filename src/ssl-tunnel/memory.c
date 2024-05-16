#include <ssl-tunnel/memory.h>

#include <stdlib.h>
#include <string.h>

void alloc_pool_free(alloc_pool_t *p) {
    _object_t *objs = (_object_t *) p->objs.array;

    for (int i = 0; i < p->objs.len; i++) {
        free(objs[i].ptr);
    }
}

void alloc_pool_init(alloc_pool_t *p) {
    memset(p, 0, sizeof(alloc_pool_t));

    slice_init(&p->objs, sizeof(_object_t), (optional_alloc_t) {
            .present = false,
    });
}

typedef struct {
    malloc_t fn;
    alloc_pool_t *pool;
} _allocator_malloc_closure;

typedef struct {
    calloc_t fn;
    alloc_pool_t *pool;
} _allocator_calloc_closure;

typedef struct {
    realloc_t fn;
    alloc_pool_t *pool;
} _allocator_realloc_closure;

void *_allocator_malloc_fn(void *ctx, size_t size);
void *_allocator_calloc_fn(void *ctx, size_t nmemb, size_t size);
void *_allocator_realloc_fn(void *ctx, void *ptr, size_t size);

err_t alloc_pool_get_allocator(alloc_pool_t *p, alloc_t *out) {
    {
        _allocator_malloc_closure *closure = malloc(sizeof(_allocator_malloc_closure));
        if (!closure) {
            return ERROR_OUT_OF_MEMORY;
        }

        closure->fn = _allocator_malloc_fn;
        closure->pool = p;

        slice_append(&p->objs, &(_object_t) {
                .ptr = closure,
                .size = sizeof(_allocator_malloc_closure)
        });

        out->malloc = &closure->fn;
    }

    {
        _allocator_calloc_closure *closure = malloc(sizeof(_allocator_calloc_closure));
        if (!closure) {
            return ERROR_OUT_OF_MEMORY;
        }

        closure->fn = _allocator_calloc_fn;
        closure->pool = p;

        slice_append(&p->objs, &(_object_t) {
                .ptr = closure,
                .size = sizeof(_allocator_calloc_closure)
        });

        out->calloc = &closure->fn;
    }

    {
        _allocator_realloc_closure *closure = malloc(sizeof(_allocator_realloc_closure));
        if (!closure) {
            return ERROR_OUT_OF_MEMORY;
        }

        closure->fn = _allocator_realloc_fn;
        closure->pool = p;

        slice_append(&p->objs, &(_object_t) {
                .ptr = closure,
                .size = sizeof(_allocator_realloc_closure)
        });

        out->realloc = &closure->fn;
    }

    return ERROR_OK;
}

void *_allocator_malloc_fn(void *ctx, size_t size) {
    void *mem = malloc(size);
    if (!mem) {
        return mem;
    }

    _allocator_malloc_closure *c = ctx;
    err_t err = slice_append(&c->pool->objs, &(_object_t) {
            .ptr = mem,
            .size = size,
    });
    if (!ERR_OK(err)) {
        abort();
    }

    return mem;
}

void *_allocator_calloc_fn(void *ctx, size_t nmemb, size_t size) {
    void *mem = calloc(nmemb, size);
    if (!mem) {
        return mem;
    }

    _allocator_calloc_closure *c = ctx;
    err_t err = slice_append(&c->pool->objs, &(_object_t){
            .ptr = mem,
            .size = nmemb * size,
    });
    if (!ERR_OK(err)) {
        abort();
    }

    return mem;
}

void *_allocator_realloc_fn(void *ctx, void *ptr, size_t size) {
    void *mem = realloc(ptr, size);
    if (!mem) {
        return mem;
    }

    _allocator_realloc_closure *c = ctx;

    _object_t *objs = c->pool->objs.array;
    int to_update = -1;
    for (int i = 0; i < c->pool->objs.len; i++) {
        if (objs[i].ptr == ptr) {
            to_update = i;
            break;
        }
    }
    if (to_update == -1) {
        abort();
    }

    objs[to_update].ptr = mem;
    objs[to_update].size = size;

    return mem;
}
