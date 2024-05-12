#include <ssl-tunnel/memory.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>

err_t mem_alloc_mem_scope(mem_scope **m) {
    *m = malloc(sizeof(mem_scope));
    if (!*m) {
        return errno;
    }

    memset(*m, 0, sizeof(mem_scope));
    return ERROR_OK;
}

void mem_destroy_mem_scope(mem_scope *m) {
    if (m->allocated_objs) {
        object *objs = (object *) m->allocated_objs->elements;

        for (int i = 0; i < m->allocated_objs->len; i++) {
            free(objs[i].ptr);
        }

        array_destroy_slice(m->allocated_objs);
    }
    free(m);
}

err_t mem_scope_init(mem_scope *m) {
    err_t err;
    if (!ERR_OK(err = array_alloc_slice(&m->allocated_objs))) {
        return err;
    }

    slice_init(m->allocated_objs, sizeof(object));
    return ERROR_OK;
}

err_t mem_scope_alloc(mem_scope *m, void **dst, size_t size) {
    *dst = malloc(size);
    if (!*dst) {
        return errno;
    }

    err_t err;
    object el = {
            .ptr = *dst,
            .size = size
    };

    if (!ERR_OK(err = slice_append(m->allocated_objs, &el))) {
        free(*dst);
        return err;
    }

    return ERROR_OK;
}
