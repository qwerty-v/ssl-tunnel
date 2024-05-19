#include <ssl-tunnel/arrays.h>

#include <stdio.h>
#include <stdlib.h>
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
} custom_object;

void test_slice_append_custom() {
    slice_t s;
    slice_init(&s, sizeof(custom_object), default_allocator);

    custom_object test_data[] = {
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
    int len = sizeof(test_data) / sizeof(custom_object);
    for (int i = 0; i < len; i++) {
        assert(ERR_OK(slice_append(&s, &test_data[i])));

        assert(s.len <= s.cap);
        assert(s.len == i + 1);
    }

    custom_object *elements = s.array;
    for (int i = 0; i < len; i++) {
        assert(memcmp(&elements[i], &test_data[i], sizeof(custom_object)) == 0);
    }

    free(s.array);
}

int main() {
    test_slice_init();
    printf("OK test_slice_init\n");
    test_slice_append_int();
    printf("OK test_slice_append_int\n");
    test_slice_append_custom();
    printf("OK test_slice_append_custom\n");

    return 0;
}