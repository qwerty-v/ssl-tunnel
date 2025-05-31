#include <ssl-tunnel/backend/hashmap.h>

#include <ssl-tunnel/lib/err.h>

const err_t ERR_HASHMAP_CAP_TOO_LOW = {
        .msg = "hashmap capacity too low"
};

// djb2 hash
static void hash_fn(const void *key, size_t len, uint32_t *out_hash) {
    const uint8_t *p = key;
    uint32_t hash = 5381;
    for (size_t i = 0; i < len; i++) {
        hash = ((hash << 5) + hash) + p[i];
    }
    *out_hash = hash;
}

void hashmap_init(hashmap_t *h, const alloc_t *alloc) {
    memset(h, 0, sizeof(hashmap_t));

    h->alloc = alloc;
}

err_t hashmap_resize(hashmap_t *h, size_t new_cap) {
    if (h->cap >= new_cap) {
        return ERR_HASHMAP_CAP_TOO_LOW;
    }

    if (h->cap == 0) {
        h->entries = alloc_malloc(h->alloc, new_cap * sizeof(hashmap_entry_t));
    } else {
        h->entries = alloc_realloc(h->alloc, h->entries, new_cap * sizeof(hashmap_entry_t));
    }
    if (!h->entries) {
        panicf("out of memory");
    }

    memset(&h->entries[h->cap], 0, (new_cap - h->cap) * sizeof(hashmap_entry_t));

    h->cap = new_cap;

    return ENULL;
}

void hashmap_insert(hashmap_t *h, uint32_t key, void **value) {
    if (h->len == h->cap) {
        size_t new_cap = 2 * h->cap;
        if (new_cap == 0) {
            new_cap = 1;
        }

        err_t err = hashmap_resize(h, new_cap);
        if (!ERROR_OK(err)) {
            panicf("error resizing hashmap: %s", err.msg);
        }
    }

    uint32_t hash;
    hash_fn(&key, sizeof(uint32_t), &hash);

    int idx = hash % h->cap;
    for (size_t i = 0; h->entries[idx].key && i < h->cap; i++) {
        if (h->entries[idx].key == key) {
            panicf("key already present");
        }

        idx++;
        idx %= h->cap;
    }

    if (h->entries[idx].key) {
        panicf("hashmap is full");
    }

    h->entries[idx].key = key;
    h->entries[idx].value = *value;

    h->len++;
}

void hashmap_get(const hashmap_t *h, uint32_t key, void **out_value, bool *out_ok) {
    *out_ok = false;

    uint32_t hash;
    hash_fn(&key, sizeof(uint32_t), &hash);

    int idx = hash % h->cap;
    for (size_t i = 0; i < h->cap; i++) {
        if (h->entries[idx].key == key) {
            *out_value = h->entries[idx].value;
            *out_ok = true;
        }

        idx++;
        idx %= h->cap;
    }
}
