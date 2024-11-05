#include <ssl-tunnel/lib/arrays.h>
#include <ssl-tunnel/lib/errors.h>

#include <stdint.h>

const err_t ERR_NEW_CAP_TOO_LOW = {
        .msg = "new capacity is too low"
};

const err_t ERR_INDEX_OUT_OF_BOUNDS = {
        .msg = "index is out of bounds"
};

static err_t _slice_check_bounds(const slice_t *s, int i);

void slice_init(slice_t *s, size_t element_size, const alloc_t *a) {
    memset(s, 0, sizeof(slice_t));

    s->element_size = element_size;
    s->allocator = a;
}

err_t slice_resize(slice_t *s, size_t new_cap) {
    if (s->cap >= new_cap) {
        return ERR_NEW_CAP_TOO_LOW;
    }

    if (s->cap == 0) {
        s->array = alloc_malloc(s->allocator, new_cap * s->element_size);
    } else {
        s->array = alloc_realloc(s->allocator, s->array, new_cap * s->element_size);
    }
    if (!s->array) {
        panicf("out of memory");
    }

    s->cap = new_cap;

    return ENULL;
}

err_t slice_append(slice_t *s, const void *element) {
    if (s->cap == 0) {
        err_t err = slice_resize(s, 1);
        if (!ERROR_OK(err)) {
            return err;
        }
    }

    if (s->len == s->cap) {
        err_t err = slice_resize(s, 2 * s->cap);
        if (!ERROR_OK(err)) {
            return err;
        }
    }

    void *dst = (uint8_t *) s->array + s->len * s->element_size;
    memcpy(dst, element, s->element_size);
    s->len++;

    return ENULL;
}

err_t slice_remove(slice_t *s, int i) {
    err_t err = _slice_check_bounds(s, i);
    if (!ERROR_OK(err)) {
        return err;
    }

    // element to be removed isn't the last element?
    if (i != (int)s->len - 1) {
        memmove(
                (uint8_t *) s->array + i * s->element_size,
                (uint8_t *) s->array + (i + 1) * s->element_size,
                (s->len - 1 - i) * s->element_size
        );
    }
    s->len--;

    return ENULL;
}

err_t slice_ith(const slice_t *s, int i, void *out) {
    err_t err = _slice_check_bounds(s, i);
    if (!ERROR_OK(err)) {
        return err;
    }

    memcpy(out, (uint8_t *) s->array + i * s->element_size, sizeof(s->element_size));
    return ENULL;
}

static err_t _slice_check_bounds(const slice_t *s, int i) {
    if (!(0 <= i && (size_t)i < s->len)) {
        return ERR_INDEX_OUT_OF_BOUNDS;
    }

    return ENULL;
}
