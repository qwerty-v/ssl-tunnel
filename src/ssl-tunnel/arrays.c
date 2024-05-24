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

    if (s->cap == 0) {
        s->array = allocator_call_malloc(s->allocator, new_cap * s->element_size);
    } else {
        s->array = allocator_call_realloc(s->allocator, s->array, new_cap * s->element_size);
    }
    if (!s->array) {
        panicf("out of memory");
    }

    s->cap = new_cap;

    return ERROR_OK;
}

err_t slice_append(slice_t *s, const void *element) {
    err_t err;
    if (s->cap == 0) {
        if (!ERR_OK(err = slice_resize(s, 1))) {
            return err;
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

err_t _slice_check_bounds(const slice_t *s, int i) {
    if (!(0 <= i && i < s->len)) {
        return ERROR_INDEX_OUT_OF_BOUNDS;
    }

    return ERROR_OK;
}

err_t slice_remove(slice_t *s, int i) {
    err_t err;
    if (!ERR_OK(err = _slice_check_bounds(s, i))) {
        return err;
    }

    // element to be removed isn't the last element?
    if (i != s->len - 1) {
        memmove(
                (uint8_t *) s->array + i * s->element_size,
                (uint8_t *) s->array + (i + 1) * s->element_size,
                (s->len - 1 - i) * s->element_size
        );
    }
    s->len--;

    return ERROR_OK;
}

err_t slice_ith(const slice_t *s, int i, void *out) {
    err_t err;
    if (!ERR_OK(err = _slice_check_bounds(s, i))) {
        return err;
    }

    memcpy(out, (uint8_t *) s->array + i * s->element_size, sizeof(s->element_size));
    return ERROR_OK;
}
