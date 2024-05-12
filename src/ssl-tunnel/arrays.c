#include <ssl-tunnel/arrays.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>

err_t array_alloc_slice(slice **s) {
    *s = (slice *) malloc(sizeof(slice));
    if (!*s) {
        return errno;
    }

    memset(*s, 0, sizeof(slice));
    return ERROR_OK;
}

void array_destroy_slice(slice *s) {
    if (s->cap) {
        free(s->elements);
    }
    free(s);
}

void slice_init(slice *s, size_t element_size) {
    s->element_size = element_size;
}

err_t slice_append(slice *s, void *element) {
    if (s->cap == 0) {
        s->elements = malloc(s->element_size);
        s->len = 0;
        s->cap = 1;
    }

    if (s->len == s->cap) {
        s->cap *= 2;
        s->elements = realloc(s->elements, s->cap * s->element_size);
    }

    memcpy((uint8_t *)s->elements + s->len * s->element_size, element, s->element_size);
    s->len++;

    return ERROR_OK;
}
