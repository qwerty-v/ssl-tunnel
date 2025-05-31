#include <ssl-tunnel/lib/slice.h>
#include <ssl-tunnel/lib/alloc.h>
#include <ssl-tunnel/lib/err.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

void test_slice_init() {
    slice_any_t s;
    slice_init(&s, 123, &alloc_std);

    assert(s.element_size == 123);
    assert(s.cap == 0);
}

/*
 * tests slice_t for sequential appending of an int,
 * asserts equality of the appended data
 */
void test_slice_append_int() {
    slice_t(int) s;
    slice_init((slice_any_t  *) &s, sizeof(int), &alloc_std);

    int test_data[] = {1, 2, 3, 4, 5};
    for (size_t i = 0; i < sizeof(test_data) / sizeof(int); i++) {
        slice_append((slice_any_t *) &s, &test_data[i]);

        assert(s.len <= s.cap);
        assert(s.len == i + 1);
    }

    for (size_t i = 0; i < s.len; i++) {
        assert(s.array[i] == test_data[i]);
    }

    free(s.array);
}

typedef struct {
    int foo;
    char *bar;
} custom_object_t;

/*
 * tests slice_t for sequential appending of a custom_object_t,
 * asserts equality of the appended data
 */
void test_slice_append_custom() {
    slice_t(custom_object_t) s;
    slice_init((slice_any_t *) &s, sizeof(custom_object_t), &alloc_std);

    custom_object_t test_data[] = {
            {
                .foo = 1,
                .bar = "abcd"
            },
            {
                .foo = 2,
                .bar = "efgh.+-"
            },
            {
                .foo = 3,
                .bar = "ijkl123"
            },
            {
                .foo = 4,
                .bar = "mnop321"
            },
            {
                .foo = 5,
                .bar = "qrstuvwxyz"
            }
    };

    for (size_t i = 0; i < sizeof(test_data) / sizeof(custom_object_t); i++) {
        slice_append((slice_any_t *) &s, &test_data[i]);

        assert(s.len <= s.cap);
        assert(s.len == i + 1);
    }

    for (size_t i = 0; i < s.len; i++) {
        assert(memcmp(&s.array[i], &test_data[i], sizeof(custom_object_t)) == 0);
    }

    free(s.array);
}

void test_slice_resize() {
    slice_t(uint8_t) s;
    slice_init((slice_any_t *) &s, sizeof(uint8_t), &alloc_std);
    assert(s.array == 0);
    assert(s.cap == 0);

    assert(ERROR_OK(slice_resize((slice_any_t *) &s, 1)));
    assert(s.array != 0);
    assert(s.cap == 1);

    assert(ERROR_OK(slice_resize((slice_any_t *) &s, 2)));
    assert(s.cap == 2);

    assert(ERROR_OK(slice_resize((slice_any_t *) &s, 50)));
    assert(s.cap == 50);

    free(s.array);
}

void test_slice_ith() {
    slice_t(int) s;
    slice_init((slice_any_t  *) &s, sizeof(int), &alloc_std);

    slice_append((slice_any_t *) &s, &(int) {5});

    int n;
    assert(ERROR_OK(slice_ith((slice_any_t *) &s, 0, &n)));
    assert(n == 5);

    slice_append((slice_any_t *) &s, &(int) {10});

    assert(ERROR_OK(slice_ith((slice_any_t *) &s, 1, &n)));
    assert(n == 10);

    assert(!ERROR_OK(slice_ith((slice_any_t *) &s, 2, 0)));

    free(s.array);
}

void test_slice_remove() {
    slice_t(int) s;
    slice_init((slice_any_t  *) &s, sizeof(int), &alloc_std);

    slice_append((slice_any_t *) &s, &(int) {5});
    slice_append((slice_any_t *) &s, &(int) {10});

    assert(s.len == 2);

    int n;
    assert(ERROR_OK(slice_ith((slice_any_t *) &s, 0, &n)));
    assert(n == 5);

    assert(ERROR_OK(slice_remove((slice_any_t *) &s, 0)));

    assert(s.len == 1);

    assert(ERROR_OK(slice_ith((slice_any_t *) &s, 0, &n)));
    assert(n == 10);

    free(s.array);
}

int main() {
    test_slice_init();
    printf("OK test_slice_init\n");
    test_slice_append_int();
    printf("OK test_slice_append_int\n");
    test_slice_append_custom();
    printf("OK test_slice_append_custom\n");
    test_slice_resize();
    printf("OK test_slice_resize\n");
    test_slice_ith();
    printf("OK test_slice_ith\n");
    test_slice_remove();
    printf("OK test_slice_remove\n");

    printf("All slice.h tests passed!\n");
    return 0;
}