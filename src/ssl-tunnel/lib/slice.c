#include <ssl-tunnel/lib/slice.h>
#include <ssl-tunnel/lib/err.h>

#include <stdint.h>
#include <stdbool.h>

const err_t ERR_SLICE_CAP_TOO_LOW = {
        .msg = "new slice capacity is too low"
};

const err_t ERR_SLICE_INDEX_OUT_OF_BOUNDS = {
        .msg = "index is out of bounds"
};

static bool _slice_can_access(const slice_t *s, int i);

void slice_init(slice_t *s, size_t element_size, const alloc_t *a) {
    memset(s, 0, sizeof(slice_t));

    s->element_size = element_size;
    s->allocator = a;
}

err_t slice_resize(slice_t *s, size_t new_cap) {
    if (s->cap >= new_cap) {
        return ERR_SLICE_CAP_TOO_LOW;
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

void slice_append(slice_t *s, const void *element) {
    if (s->len == s->cap) {
        int new_cap = 2 * s->cap;
        if (new_cap == 0) {
            new_cap = 1;
        }

        err_t err = slice_resize(s, new_cap);
        if (!ERROR_OK(err)) {
            panicf("error resizing slice: %s", err.msg);
        }
    }

    void *dst = (uint8_t *) s->array + s->len * s->element_size;
    memcpy(dst, element, s->element_size);
    s->len++;
}

err_t slice_remove(slice_t *s, int i) {
    if (!_slice_can_access(s, i)) {
        return ERR_SLICE_INDEX_OUT_OF_BOUNDS;
    }

    // removing last element?
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
    if (!_slice_can_access(s, i)) {
        return ERR_SLICE_INDEX_OUT_OF_BOUNDS;
    }

    memcpy(out, (uint8_t *) s->array + i * s->element_size, sizeof(s->element_size));
    return ENULL;
}

static bool _slice_can_access(const slice_t *s, int i) {
    return 0 <= i && (size_t)i < s->len;
}
