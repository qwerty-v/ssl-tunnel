#include <ssl-tunnel/memory.h>

#include <stdlib.h>
#include <string.h>

void alloc_pool_free(alloc_pool_t *p) {
    _object_t *objs = p->objs.array;
    for (int i = 0; i < p->objs.len; i++) {
        free(objs[i].ptr);
    }

    if (p->objs.cap) {
        free(objs);
    }
}

void alloc_pool_init(alloc_pool_t *p) {
    memset(p, 0, sizeof(alloc_pool_t));

    slice_init(&p->objs, sizeof(_object_t), (optional_alloc_t) {
            .present = false,
    });
}

err_t alloc_pool_append_obj(alloc_pool_t *p, void *ptr, size_t size) {
    return slice_append(&p->objs, &(_object_t) {
            .ptr = ptr,
            .size = size
    });
}

typedef struct {
    void *fn;
    alloc_pool_t *pool;
} allocator_closure_t;

void *_allocator_calloc_fn(void *closure, size_t nmemb, size_t size) {
    void *p = calloc(nmemb, size);
    if (!p) {
        panicf("out of memory");
    }

    allocator_closure_t *c = closure;
    if (!ERR_OK(alloc_pool_append_obj(c->pool, p, nmemb * size))) {
        panicf("could not append allocated memory");
    }

    return p;
}

void *_allocator_malloc_fn(void *closure, size_t size) {
    void *p = malloc(size);
    if (!p) {
        panicf("out of memory");
    }

    allocator_closure_t *c = closure;
    if (!ERR_OK(alloc_pool_append_obj(c->pool, p, size))) {
        panicf("could not append allocated memory");
    }

    return p;
}

void *_allocator_realloc_fn(void *closure, void *ptr, size_t size) {
    void *p = realloc(ptr, size);
    if (!p) {
        panicf("out of memory");
    }

    allocator_closure_t *c = closure;

    _object_t *objs = c->pool->objs.array;
    int to_update = -1;
    for (int i = 0; i < c->pool->objs.len; i++) {
        if (objs[i].ptr == ptr) {
            to_update = i;
            break;
        }
    }
    if (to_update == -1) {
        panicf("call to realloc without prior allocation");
    }

    objs[to_update].ptr = p;
    objs[to_update].size = size;

    return p;
}

err_t alloc_pool_get_allocator(alloc_pool_t *p, alloc_t *out) {
    allocator_closure_t *closures = malloc(3 * sizeof(allocator_closure_t));
    if (!closures) {
        panicf("out of memory");
    }

    err_t err = slice_append(&p->objs, &(_object_t) {
            .ptr = closures,
            .size = 3 * sizeof(allocator_closure_t)
    });
    if (!ERR_OK(err)) {
        return err;
    }

    closures[0].fn = _allocator_calloc_fn;
    closures[0].pool = p;
    closures[1].fn = _allocator_malloc_fn;
    closures[1].pool = p;
    closures[2].fn = _allocator_realloc_fn;
    closures[2].pool = p;

    out->calloc = (calloc_t * ) & closures[0].fn;
    out->malloc = (malloc_t * ) & closures[1].fn;
    out->realloc = (realloc_t * ) & closures[2].fn;
    return ERROR_OK;
}
