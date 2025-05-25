#pragma once

#include <ssl-tunnel/lib/alloc.h>
#include <ssl-tunnel/lib/err.h>
#include <ssl-tunnel/lib/slice.h> // fixme

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    const alloc_t *alloc;
    slice_t(void *) arr;
} trie_t;

void trie_init(trie_t *t, const alloc_t *alloc);

void trie_insert(trie_t *t, uint32_t addr, uint8_t addr_prefix, void *v);

void trie_match(const trie_t *t, uint32_t lookup_ip, void **out_v, bool *ok);