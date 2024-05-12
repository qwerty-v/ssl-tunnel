#include <ssl-tunnel/arrays.h>

#include <stdio.h>
#include <assert.h>
#include <string.h>

void test_alloc() {
    slice *s;

    assert(array_alloc_slice(&s) == ERROR_OK);
    assert(s->cap == 0);
    assert(s->len == 0);

    array_destroy_slice(s);
}

void test_init() {
    slice *s;

    assert(array_alloc_slice(&s) == ERROR_OK);
    slice_init(s, 123);
    assert(s->element_size == 123);

    array_destroy_slice(s);
}

void test_append_int() {
    slice *s;

    assert(array_alloc_slice(&s) == ERROR_OK);
    slice_init(s, sizeof(int));

    int test_data[] = {1, 2, 3, 4, 5};
    int len = sizeof(test_data) / sizeof(int);
    for (int i = 0; i < len; i++) {
        int el = test_data[i];
        assert(slice_append(s, &el) == ERROR_OK);

        assert(s->len <= s->cap);
        assert(s->len == i + 1);
    }


    int *elements = (int *) s->elements;
    for (int i = 0; i < len; i++) {
        assert(elements[i] == test_data[i]);
    }

    array_destroy_slice(s);
}

typedef struct {
    int foo;
    char *bar;
} custom_object;

void test_append_custom_struct() {
    slice *s;

    assert(array_alloc_slice(&s) == ERROR_OK);
    slice_init(s, sizeof(custom_object));

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
        custom_object el = test_data[i];
        assert(slice_append(s, &el) == ERROR_OK);

        assert(s->len <= s->cap);
        assert(s->len == i + 1);
    }


    custom_object *elements = (custom_object *) s->elements;
    for (int i = 0; i < len; i++) {
        assert(elements[i].foo == test_data[i].foo);
        assert(strcmp(elements[i].bar, test_data[i].bar) == 0);
    }

    array_destroy_slice(s);
}

int main() {
    test_alloc();
    printf("OK test_alloc\n");
    test_init();
    printf("OK test_init\n");
    test_append_int();
    printf("OK test_append_int\n");
    test_append_custom_struct();
    printf("OK test_append_custom_struct\n");

    return 0;
}