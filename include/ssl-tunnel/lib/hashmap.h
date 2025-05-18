#pragma once

#include <ssl-tunnel/lib/alloc.h>
#include <ssl-tunnel/lib/err.h>

typedef struct {
    void *key;
    int len;
} hashmap_key_t;

typedef struct {
    hashmap_key_t key;
    void *v;
} hashmap_entry_t;

typedef struct {
    int cap;
    int len;
    const alloc_t *alloc;
    hashmap_entry_t  *entries;
} hashmap_t;

void hashmap_init(hashmap_t *h, const alloc_t *alloc);

void hashmap_insert();
err_t hashmap_get();