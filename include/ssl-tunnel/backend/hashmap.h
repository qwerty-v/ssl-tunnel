#pragma once

#include <ssl-tunnel/backend/peer.h>

#include <ssl-tunnel/lib/alloc.h>
#include <ssl-tunnel/lib/err.h>

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t key;
    peer_t *value;
} hashmap_entry_t;

typedef struct {
    size_t cap;
    size_t len;
    const alloc_t *alloc;
    hashmap_entry_t *entries;
} hashmap_t;

void hashmap_init(hashmap_t *h, const alloc_t *alloc);

err_t hashmap_resize(hashmap_t *h, size_t new_cap);

void hashmap_insert(hashmap_t *h, uint32_t key, peer_t **value);

void hashmap_get(const hashmap_t *h, uint32_t key, peer_t **out_value, bool *out_ok);