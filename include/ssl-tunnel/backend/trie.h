#pragma once

#include <ssl-tunnel/backend/peer.h>

#include <ssl-tunnel/lib/alloc.h>
#include <ssl-tunnel/lib/err.h>
#include <ssl-tunnel/lib/slice.h> // fixme

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    const alloc_t *alloc;
    slice_t(peer_t *) arr;
} trie_t;

void trie_init(trie_t *t, const alloc_t *alloc);

void trie_insert(trie_t *t, uint32_t addr, uint8_t addr_prefix, peer_t **v);

void trie_match(const trie_t *t, uint32_t lookup_ip, peer_t **out_v, bool *ok);