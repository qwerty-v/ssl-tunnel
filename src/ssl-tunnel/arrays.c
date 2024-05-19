#include <ssl-tunnel/arrays.h>

#include <string.h>
#include <stdint.h>

const err_t ERROR_NEW_CAP_TOO_LOW = {
        .ok = false,
        .msg = "new capacity is too low"
};

const err_t ERROR_INDEX_OUT_OF_BOUNDS = {
        .ok = false,
        .msg = "index is out of bounds"
};

void slice_init(slice_t *s, size_t element_size, optional_alloc_t allocator) {
    memset(s, 0, sizeof(slice_t));

    s->element_size = element_size;
    if (allocator.present) {
        s->allocator = allocator.value;
    } else {
        s->allocator = std_alloc;
    }
}

err_t slice_resize(slice_t *s, int new_cap) {
    if (s->cap >= new_cap) {
        return ERROR_NEW_CAP_TOO_LOW;
    }

    s->cap = new_cap;

    s->array = allocator_call_realloc(s->allocator, s->array, s->cap * s->element_size);
    if (!s->array) {
        return ERROR_OUT_OF_MEMORY;
    }

    return ERROR_OK;
}

err_t slice_append(slice_t *s, const void *element) {
    err_t err;

    if (s->cap == 0) {
        s->len = 0;
        s->cap = 1;

        s->array = allocator_call_malloc(s->allocator, s->element_size);
        if (!s->array) {
            return ERROR_OUT_OF_MEMORY;
        }
    }

    if (s->len == s->cap) {
        if (!ERR_OK(err = slice_resize(s, 2 * s->cap))) {
            return err;
        }
    }

    void *dst = (uint8_t *) s->array + s->len * s->element_size;
    memcpy(dst, element, s->element_size);
    s->len++;

    return ERROR_OK;
}

err_t slice_remove(slice_t *s, int i) {
    if (!(0 <= i && i < s->len)) {
        return ERROR_INDEX_OUT_OF_BOUNDS;
    }

    if (i == s->len - 1) {
        s->len--;
        return ERROR_OK;
    }

    memmove(
            (uint8_t *) s->array + i * s->element_size,
            (uint8_t *) s->array + (i + 1) * s->element_size,
            (s->len - 1 - i) * s->element_size
    );
    return ERROR_OK;
}

err_t slice_ith(slice_t *s, int i, void *out) {
    if (!(0 <= i && i < s->len)) {
        return ERROR_INDEX_OUT_OF_BOUNDS;
    }

    memcpy(out, (uint8_t *) s->array + i * s->element_size, sizeof(s->element_size));
    return ERROR_OK;
}
