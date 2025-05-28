#include <ssl-tunnel/lib/deque.h>
#include <ssl-tunnel/lib/alloc.h>

#include <string.h>

const err_t ERR_DEQUE_CAP_TOO_LOW = {
        .msg = "new deque capacity is too low"
};

const err_t ERR_DEQUE_EMPTY = {
        .msg = "deque is empty"
};

void deque_init(deque_any_t *d, size_t element_size, const alloc_t *a) {
    memset(d, 0, sizeof(deque_any_t));

    d->element_size = element_size;
    d->alloc = a;
}

err_t deque_resize(deque_any_t *d, size_t new_cap) {
    if (d->cap >= new_cap) {
        return ERR_DEQUE_CAP_TOO_LOW;
    }

    uint8_t *new_arr = alloc_malloc(d->alloc, new_cap * d->element_size);

    for (size_t i = 0; i < d->len; i++) {
        size_t old_ind = (d->front + i) % d->cap;

        memcpy(new_arr + i * d->element_size, d->array + old_ind * d->element_size, d->element_size);
    }

    if (d->cap > 0) {
        alloc_free(d->alloc, d->array);
    }

    d->array = new_arr;
    d->front = 0;
    d->back = 0;
    if (d->len != 0) {
        assert(d->len > 0);
        d->back = d->len - 1;
    }
    d->cap = new_cap;

    return ENULL;
}

void deque_push_back(deque_any_t *d, const void *element) {
    if (d->len == d->cap) {
        size_t new_cap = 2 * d->cap;
        if (new_cap == 0) {
            new_cap = 1;
        }

        err_t err = deque_resize(d, new_cap);
        if (!ERROR_OK(err)) {
            panicf("error resizing deque: %s", err.msg);
        }
    }

    if (d->len != 0) {
        d->back++;
        d->back %= d->cap;
    } else {
        assert(d->front == d->back);
        assert(0 <= d->front && d->front < d->cap);
    }

    void *dst = (uint8_t *) d->array + d->back * d->element_size;
    memcpy(dst, element, d->element_size);
    d->len++;
}

void deque_push_front(deque_any_t *d, const void *element) {
    if (d->len == d->cap) {
        size_t new_cap = 2 * d->cap;
        if (new_cap == 0) {
            new_cap = 1;
        }

        err_t err = deque_resize(d, new_cap);
        if (!ERROR_OK(err)) {
            panicf("error resizing deque: %s", err.msg);
        }
    }

    if (d->len != 0) {
        // d->front--;
        d->front = (d->front + d->cap - 1) % d->cap;
    } else {
        assert(d->front == d->back);
        assert(0 <= d->front && d->front < d->cap);
    }

    void *dst = (uint8_t *) d->array + d->front * d->element_size;
    memcpy(dst, element, d->element_size);
    d->len++;
}

err_t deque_back(deque_any_t *d, void *out_dst) {
    if (d->len == 0) {
        return ERR_DEQUE_EMPTY;
    }

    memcpy(out_dst, d->array + d->back * d->element_size, d->element_size);

    return ENULL;
}

err_t deque_pop_back(deque_any_t *d) {
    if (d->len == 0) {
        return ERR_DEQUE_EMPTY;
    }

    d->len--;

    if (d->len != 0) {
        // d->back--;
        d->back = (d->back + d->cap - 1) % d->cap;
    } else {
        assert(d->front == d->back);
        assert(0 <= d->front && d->front < d->cap);
    }

    return ENULL;
}

err_t deque_front(deque_any_t *d, void *out_dst) {
    if (d->len == 0) {
        return ERR_DEQUE_EMPTY;
    }

    memcpy(out_dst, d->array + d->front * d->element_size, d->element_size);

    return ENULL;
}

err_t deque_pop_front(deque_any_t *d) {
    if (d->len == 0) {
        return ERR_DEQUE_EMPTY;
    }

    d->len--;

    if (d->len != 0) {
        d->front++;
        d->front %= d->cap;
    } else {
        assert(d->front == d->back);
        assert(0 <= d->front && d->front < d->cap);
    }

    return ENULL;
}
