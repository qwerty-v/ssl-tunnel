#include <ssl-tunnel/backend/trie.h>

void trie_init(trie_t *t, const alloc_t *alloc) {
    memset(t, 0, sizeof(trie_t));

    t->alloc = alloc;

    slice_init((slice_any_t *) &t->arr, sizeof(void *), alloc);
}

void trie_insert(trie_t *t, uint32_t addr, uint8_t addr_prefix, peer_t **v) {
    // fixme
    (void) addr;
    (void) addr_prefix;

    slice_append((slice_any_t *) &t->arr, v);
}

void trie_match(const trie_t *t, uint32_t lookup_ip, peer_t **out_v, bool *ok) {
    // fixme
    (void) lookup_ip;

    *ok = true;
    *out_v = t->arr.array[0];
}