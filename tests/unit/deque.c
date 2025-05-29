#include <ssl-tunnel/lib/deque.h>
#include <ssl-tunnel/lib/memscope.h>

#include <stdio.h>
#include <assert.h>
#include <string.h>

void test_deque_init() {
    deque_t(int) d;
    deque_init((deque_any_t *) &d, sizeof(int), NULL);

    assert(d.cap == 0);
    assert(d.len == 0);
    assert(d.element_size == sizeof(int));
    assert(d.alloc == NULL);
    assert(d.front == 0);
    assert(d.back == 0);
    assert(d.array == NULL);
}

// push_back & pop_back
void test_push_pop_back() {
    memscope_t m;
    memscope_init(&m);

    deque_t(int) d;
    deque_init((deque_any_t *) &d, sizeof(int), &m.alloc);

    int val = 10;
    deque_push_back((deque_any_t *) &d, &val);
    assert(d.cap >= d.len);
    assert(d.len == 1);
    assert(d.front == 0);
    assert(d.back == 0);

    int out;
    assert(ERROR_OK(deque_back((deque_any_t *) &d, &out)));
    assert(out == 10);
    assert(d.len == 1);

    assert(ERROR_OK(deque_pop_back((deque_any_t *) &d)));
    assert(d.len == 0);

    memscope_free(&m);
}

// push_front & pop_front
void test_push_pop_front() {
    memscope_t m;
    memscope_init(&m);

    deque_t(int) d;
    deque_init((deque_any_t *) &d, sizeof(int), &m.alloc);

    int val = 20;
    deque_push_front((deque_any_t *) &d, &val);
    assert(d.cap >= d.len);
    assert(d.len == 1);
    assert(d.front == 0);
    assert(d.back == 0);

    int out;
    assert(ERROR_OK(deque_front((deque_any_t *) &d, &out)));
    assert(out == 20);
    assert(d.len == 1);

    assert(ERROR_OK(deque_pop_front((deque_any_t *) &d)));
    assert(d.len == 0);

    memscope_free(&m);
}

// push_front & push_back
void test_mixed_operations_1() {
    memscope_t m;
    memscope_init(&m);

    deque_t(int) d;
    deque_init((deque_any_t *) &d, sizeof(int), &m.alloc);

    int vals[] = {1, 2, 3, 4, 5, 6};
    for (int i = 3; i < 6; i++) {
        deque_push_back((deque_any_t *) &d, &vals[i]);
    }
    for (int i = 2; i >= 0; i--) {
        deque_push_front((deque_any_t *) &d, &vals[i]);
    }
    assert(d.len == 6);


    for (int i = 0; i < 6; i++) {
        int v;
        assert(ERROR_OK(deque_front((deque_any_t *) &d, &v)));
        assert(v == vals[i]);

        assert((int)d.len == 6 - i);
        assert(ERROR_OK(deque_pop_front((deque_any_t *) &d)));
        assert((int)d.len == 6 - i - 1);
    }

    memscope_free(&m);
}

// push_front & push_back
void test_mixed_operations_2() {
    memscope_t m;
    memscope_init(&m);

    deque_t(int) d;
    deque_init((deque_any_t *) &d, sizeof(int), &m.alloc);

    int vals[] = {1, 2, 3, 4, 5, 6};
    for (int i = 0; i < 3; i++) {
        deque_push_back((deque_any_t *) &d, &vals[2 - i]);
        deque_push_front((deque_any_t *) &d, &vals[3 + i]);
    }
    assert(d.len == 6);

    for (int i = 0; i < 6; i++) {
        int v;
        assert(ERROR_OK(deque_back((deque_any_t *) &d, &v)));
        assert(v == vals[i]);

        assert(d.len == 6 - i);
        assert(ERROR_OK(deque_pop_back((deque_any_t *) &d)));
        assert(d.len == 6 - i - 1);
    }

    memscope_free(&m);
}

void test_resize() {
    memscope_t m;
    memscope_init(&m);

    deque_t(int) d;
    deque_init((deque_any_t *) &d, sizeof(int), &m.alloc);

    const int N = 1000;
    for (int i = 0; i < N; i++) {
        deque_push_back((deque_any_t *) &d, &i);
    }
    assert((int)d.len == N);

    int out;
    assert(ERROR_OK(deque_front((deque_any_t *) &d, &out)));
    assert(out == 0);
    assert(ERROR_OK(deque_back((deque_any_t *) &d, &out)));
    assert(out == N - 1);

    for (int i = 0; i < N; i++) {
        deque_pop_front((deque_any_t *) &d);
    }
    assert(d.len == 0);

    memscope_free(&m);
}

void test_errors_on_empty() {
    memscope_t m;
    memscope_init(&m);

    deque_t(int) d;
    deque_init((deque_any_t *) &d, sizeof(int), &m.alloc);

    int out;
    assert(!ERROR_OK(deque_front((deque_any_t *) &d, &out)));

    assert(!ERROR_OK(deque_back((deque_any_t *) &d, &out)));

    assert(!ERROR_OK(deque_pop_front((deque_any_t *) &d)));

    assert(!ERROR_OK(deque_pop_back((deque_any_t *) &d)));

    memscope_free(&m);
}

void test_wraparound() {
    memscope_t m;
    memscope_init(&m);

    deque_t(int) d;
    deque_init((deque_any_t *) &d, sizeof(int), &m.alloc);

    int vals[] = {1, 2, 3, 4};
    for (int i = 0; i < 4; i++) {
        deque_push_back((deque_any_t *) &d, &vals[i]);
    }
    assert(d.len == 4);

    deque_pop_front((deque_any_t *) &d);
    deque_pop_front((deque_any_t *) &d);
    assert(d.front == 2);
    assert(d.len == 2);

    int val5 = 5;
    deque_push_back((deque_any_t *) &d, &val5);
    int val6 = 6;
    deque_push_back((deque_any_t *) &d, &val6);
    assert(d.back == 1);

    int expected[] = {3, 4, 5, 6};
    for (int i = 0; i < 4; i++) {
        int v;
        deque_front((deque_any_t *) &d, &v);
        assert(v == expected[i]);
        deque_pop_front((deque_any_t *) &d);
    }

    memscope_free(&m);
}

void test_data_integrity_after_resize() {
    memscope_t m;
    memscope_init(&m);

    deque_t(int) d;
    deque_init((deque_any_t *) &d, sizeof(int), &m.alloc);

    for (int i = 0; i < 10; i++) {
        deque_push_back((deque_any_t *) &d, &i);
    }

    deque_resize((deque_any_t *) &d, 20);
    assert(d.cap == 20);

    // Проверка элементов
    for (int i = 9; i >= 0; i--) {
        int v;
        assert(ERROR_OK(deque_back((deque_any_t *) &d, &v)));
        assert(v == i);

        assert(ERROR_OK(deque_pop_back((deque_any_t *) &d)));
    }

    memscope_free(&m);
}

int main() {
    test_deque_init();
    printf("test_deque_init passed\n");
    test_push_pop_back();
    printf("test_push_pop_back passed\n");
    test_push_pop_front();
    printf("test_push_pop_front passed\n");
    test_mixed_operations_1();
    printf("test_mixed_operations_1 passed\n");
    test_mixed_operations_2();
    printf("test_mixed_operations_2 passed\n");
    test_resize();
    printf("test_resize passed\n");
    test_errors_on_empty();
    printf("test_errors_on_empty passed\n");
    test_wraparound();
    printf("test_wraparound passed\n");
    test_data_integrity_after_resize();
    printf("test_data_integrity_after_resize passed\n");

    printf("All deque.c tests passed!\n");
    return 0;
}