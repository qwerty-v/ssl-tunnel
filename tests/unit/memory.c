#include <ssl-tunnel/lib/memscope.h>

#include <stdio.h>
#include <assert.h>

void test_alloc_pool_init() {
    alloc_pool_t p;
    alloc_pool_init(&p);

    assert(p.objs.cap == 0);
}

void test_alloc_pool_allocator() {
    alloc_pool_t p;
    alloc_pool_init(&p);

    alloc_t allocator;
    assert(ERR_OK(alloc_pool_get_allocator(&p, &allocator)));

    int old_len = p.objs.len;
    int iterations = 10;
    for (int i = 0; i < iterations; i++) {
        void *m = allocator_call_malloc(allocator, sizeof(int));
        void *c = allocator_call_calloc(allocator, 1, sizeof(int));
        void *r = allocator_call_realloc(allocator, m, 2 * sizeof(int));

        assert(!!m && !!c && !!r);
    }
    assert(p.objs.len - old_len == iterations * 2);

    int *n = allocator_call_calloc(allocator, 1, sizeof(int));
    assert(!!n && *n == 0);

    *n = 123;
    _object_t *obj_array = p.objs.array;
    _object_t last = obj_array[p.objs.len - 1];

    assert(*((int *)last.ptr) == 123);

    alloc_pool_free(&p);
}

int main() {
    test_alloc_pool_init();
    printf("OK test_alloc_pool_init\n");
    test_alloc_pool_allocator();
    printf("OK test_alloc_pool_allocator\n");
    return 0;
}