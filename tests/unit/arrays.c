#include <ssl-tunnel/arrays.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

optional_alloc_t default_allocator = {
        .present = false
};

void test_slice_init() {
    slice_t s;
    slice_init(&s, 123, default_allocator);

    assert(s.element_size == 123);
    assert(s.cap == 0);
}

/*
 * tests slice_t for sequential appending of an int,
 * asserts equality of the appended data
 */
void test_slice_append_int() {
    slice_t s;
    slice_init(&s, sizeof(int), default_allocator);

    int test_data[] = {1, 2, 3, 4, 5};
    int len = sizeof(test_data) / sizeof(int);
    for (int i = 0; i < len; i++) {
        assert(ERR_OK(slice_append(&s, &test_data[i])));

        assert(s.len <= s.cap);
        assert(s.len == i + 1);
    }

    int *elements = s.array;
    for (int i = 0; i < len; i++) {
        assert(elements[i] == test_data[i]);
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
    slice_t s;
    slice_init(&s, sizeof(custom_object_t), default_allocator);

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
    int len = sizeof(test_data) / sizeof(custom_object_t);
    for (int i = 0; i < len; i++) {
        assert(ERR_OK(slice_append(&s, &test_data[i])));

        assert(s.len <= s.cap);
        assert(s.len == i + 1);
    }

    custom_object_t *elements = s.array;
    for (int i = 0; i < len; i++) {
        assert(memcmp(&elements[i], &test_data[i], sizeof(custom_object_t)) == 0);
    }

    free(s.array);
}

void test_slice_resize() {
    slice_t s;
    slice_init(&s, sizeof(uint8_t), default_allocator);
    assert(s.array == 0);
    assert(s.cap == 0);

    assert(ERR_OK(slice_resize(&s, 1)));
    assert(s.array != 0);
    assert(s.cap == 1);

    assert(ERR_OK(slice_resize(&s, 2)));
    assert(s.cap == 2);

    assert(ERR_OK(slice_resize(&s, 50)));
    assert(s.cap == 50);

    free(s.array);
}

void test_slice_ith() {
    slice_t s;
    slice_init(&s, sizeof(int), default_allocator);

    assert(ERR_OK(slice_append(&s, &(int) {5})));

    int n;
    assert(ERR_OK(slice_ith(&s, 0, &n)));
    assert(n == 5);

    assert(ERR_OK(slice_append(&s, &(int) {10})));

    assert(ERR_OK(slice_ith(&s, 1, &n)));
    assert(n == 10);

    assert(!ERR_OK(slice_ith(&s, 2, 0)));

    free(s.array);
}

void test_slice_remove() {
    slice_t s;
    slice_init(&s, sizeof(int), default_allocator);

    assert(ERR_OK(slice_append(&s, &(int) {5})));
    assert(ERR_OK(slice_append(&s, &(int) {10})));

    assert(s.len == 2);

    int n;
    assert(ERR_OK(slice_ith(&s, 0, &n)));
    assert(n == 5);

    assert(ERR_OK(slice_remove(&s, 0)));

    assert(s.len == 1);

    assert(ERR_OK(slice_ith(&s, 0, &n)));
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

    return 0;
}