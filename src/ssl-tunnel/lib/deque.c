#include <ssl-tunnel/lib/deque.h>

#include <string.h>

const err_t ERR_DEQUE_CAP_TOO_LOW = {
        .msg = "new deque capacity is too low"
};

const err_t ERR_DEQUE_EMPTY = {
        .msg = "deque is empty"
};

void deque_init(deque_t *d, size_t element_size, const alloc_t *a) {
    memset(d, 0, sizeof(deque_t));

    d->element_size = element_size;
    d->allocator = a;
}

err_t deque_resize(deque_t *d, size_t new_cap) {
    if (d->cap >= new_cap) {
        return ERR_DEQUE_CAP_TOO_LOW;
    }

    uint8_t *new_arr = alloc_malloc(d->allocator, new_cap * d->element_size);

    for (int i = 0; i < (int)d->len; i++) {
        int old_ind = (d->front + i) % d->cap;

        memcpy(new_arr + i * d->element_size, d->array + old_ind * d->element_size, d->element_size);
    }

    alloc_free(d->allocator, d->array);
    d->array = new_arr;
    d->front = 0;
    d->back = d->len - 1;
    d->cap = new_cap;

    return ENULL;
}

void deque_push_back(deque_t *d, const void *element) {
    if (d->len == d->cap) {
        int new_cap = 2 * d->cap;
        if (new_cap == 0) {
            new_cap = 1;
        }

        err_t err = deque_resize(d, new_cap);
        if (!ERROR_OK(err)) {
            panicf("error resizing deque: %s", err.msg);
        }
    }

    d->back++;
    d->back %= d->cap;

    void *dst = (uint8_t *) d->array + d->back * d->element_size;
    memcpy(dst, element, d->element_size);
    d->len++;
}

err_t deque_pop_back(deque_t *d) {
    if (d->len == 0) {
        return ERR_DEQUE_EMPTY;
    }

    d->len--;
    d->back--;
    if (d->back < 0) {
        d->back = d->cap - 1;
    }

    return ENULL;
}

void deque_push_front(deque_t *d, const void *element) {
    if (d->len == d->cap) {
        int new_cap = 2 * d->cap;
        if (new_cap == 0) {
            new_cap = 1;
        }

        err_t err = deque_resize(d, new_cap);
        if (!ERROR_OK(err)) {
            panicf("error resizing deque: %s", err.msg);
        }
    }

    d->front--;
    if (d->front < 0) {
        d->front = d->cap - 1;
    }

    void *dst = (uint8_t *) d->array + d->front * d->element_size;
    memcpy(dst, element, d->element_size);
    d->len++;
}

err_t deque_pop_front(deque_t *d) {
    if (d->len == 0) {
        return ERR_DEQUE_EMPTY;
    }

    d->len--;
    d->front++;
    d->front %= d->cap;

    return ENULL;
}
