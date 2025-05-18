#include <ssl-tunnel/lib/hashmap.h>

void hashmap_init(hashmap_t *h, const alloc_t *alloc) {
    memset(h, 0, sizeof(hashmap_t));

    h->alloc = alloc;
}

void hashmap_insert() {

}

err_t hashmap_get() {
    return ENULL;
}