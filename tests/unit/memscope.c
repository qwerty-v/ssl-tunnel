#include <ssl-tunnel/lib/memscope.h>

#include <stdio.h>
#include <assert.h>

void test_memscope_init() {
    memscope_t p;
    memscope_init(&p);

    assert(p.ptrs.cap == 0);
    assert(p.ptrs.element_size == sizeof(void *));
}

void test_memscope_alloc() {
    memscope_t p;
    memscope_init(&p);

    int old_len = p.ptrs.len;
    int iterations = 10;
    for (int i = 0; i < iterations; i++) {
        void *m = alloc_malloc(&p.alloc, sizeof(int));
        void *c = alloc_calloc(&p.alloc, 1, sizeof(int));
        void *r = alloc_realloc(&p.alloc, m, 2 * sizeof(int));

        assert(!!m && !!c && !!r);
    }
    assert(p.ptrs.len - old_len == iterations * 2);

    int *n = alloc_calloc(&p.alloc, 1, sizeof(int));
    assert(!!n && *n == 0);

    *n = 123;
    void *last = p.ptrs.array[p.ptrs.len - 1];

    assert(*((int *)last) == 123);

    memscope_free(&p);
}

int main() {
    test_memscope_init();
    printf("OK test_memscope_init\n");
    test_memscope_alloc();
    printf("OK test_memscope_alloc\n");

    printf("All memscope.h tests passed!\n");
    return 0;
}