#include <ssl-tunnel/lib/memscope.h>

#include <stdio.h>
#include <assert.h>

void test_memscope_init() {
    memscope_t p;
    memscope_init(&p);

    assert(p.ptrs.cap == 0);
    assert(p.ptrs.len == 0);
    assert(p.ptrs.element_size == sizeof(void *));
}

void test_memscope_malloc() {
    memscope_t p;
    memscope_init(&p);

    int *m = (int *)alloc_malloc(&p.alloc, sizeof(int));
    assert(m != NULL);
    *m = 42;

    assert(p.ptrs.len == 1);
    assert(p.ptrs.array[0] == m);
    assert(*(int *)p.ptrs.array[0] == 42);

    memscope_free(&p);
}

void test_memscope_calloc() {
    memscope_t p;
    memscope_init(&p);

    int *c = (int *)alloc_calloc(&p.alloc, 1, sizeof(int));
    assert(c != NULL);
    assert(*c == 0); // calloc обнуляет память

    assert(p.ptrs.len == 1);
    assert(p.ptrs.array[0] == c);

    memscope_free(&p);
}

void test_memscope_realloc() {
    memscope_t p;
    memscope_init(&p);

    int *m = (int *)alloc_malloc(&p.alloc, sizeof(int));
    assert(m != NULL);
    *m = 10;

    assert(p.ptrs.len == 1);

    int *r = (int *)alloc_realloc(&p.alloc, m, 2 * sizeof(int));
    assert(r != NULL);
    // value retained?
    assert(*r == 10);

    assert(p.ptrs.len >= 1);
    // last one should be the latest one
    assert(p.ptrs.array[p.ptrs.len - 1] == r);

    memscope_free(&p);
}

void test_memscope_complex() {
    memscope_t p;
    memscope_init(&p);

    size_t initial_len = p.ptrs.len;
    size_t alloc_count = 10;

    for (size_t i = 0; i < alloc_count; i++) {
        int *m = (int *) alloc_malloc(&p.alloc, sizeof(int));
        assert(m != NULL);
        *m = i;

        int *c = (int *) alloc_calloc(&p.alloc, 1, sizeof(int));
        assert(c != NULL);
        *c = i + alloc_count;

        int *r = (int *) alloc_realloc(&p.alloc, m, 2 * sizeof(int));
        assert(r != NULL);
        // value should retain
        assert(*r == (int)i);
    }

    // at least got bigger?
    assert(p.ptrs.len > initial_len);
    // at least 2 * alloc_count times (malloc + calloc)?
    assert(p.ptrs.len >= initial_len + 2 * alloc_count);

    // every pointer at this point should be valid
    for (size_t i = 0; i < p.ptrs.len; i++) {
        assert(p.ptrs.array[i] != NULL);
    }

    memscope_free(&p);
}

void test_memscope_free_null() {
    memscope_t p;
    memscope_init(&p);

    void *ptr = alloc_malloc(&p.alloc, sizeof(int));
    assert(ptr != NULL);

    // hate this but should work
    alloc_free(&p.alloc, NULL);

    memscope_free(&p);
}
int main() {
    test_memscope_init();
    printf("OK test_memscope_init\n");
    test_memscope_malloc();
    printf("OK test_memscope_malloc\n");
    test_memscope_calloc();
    printf("OK test_memscope_calloc\n");
    test_memscope_realloc();
    printf("OK test_memscope_realloc\n");
    test_memscope_complex();
    printf("OK test_memscope_complex\n");

    test_memscope_free_null();
    printf("OK test_memscope_free_null\n");

    printf("All memscope.h tests passed!\n");
    return 0;
}
