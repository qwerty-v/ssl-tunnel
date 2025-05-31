#include <ssl-tunnel/lib/alloc.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    int alloc_counter;
} custom_alloc_t;

void* custom_calloc(void* extra, size_t nmemb, size_t size) {
    void* ptr = calloc(nmemb, size);

    custom_alloc_t* counter = (custom_alloc_t*)extra;
    if (ptr) {
        counter->alloc_counter++;
    }

    return ptr;
}

void* custom_malloc(void* extra, size_t size) {
    void* ptr = malloc(size);

    custom_alloc_t* counter = (custom_alloc_t*)extra;
    if (ptr) {
        counter->alloc_counter++;
    }

    return ptr;
}

void* custom_realloc(void* extra, void* ptr, size_t size) {
    void* new_ptr = realloc(ptr, size);
    if (!new_ptr || new_ptr == ptr) {
        return new_ptr;
    }

    custom_alloc_t* counter = (custom_alloc_t*)extra;

    if (ptr) {
        counter->alloc_counter--;
    }
    counter->alloc_counter++;

    return new_ptr;
}

void custom_free(void* extra, void* ptr) {
    custom_alloc_t* counter = (custom_alloc_t*)extra;
    if (ptr) {
        counter->alloc_counter--;
    }

    free(ptr);
}

// test: alloc & free through alloc_std
void test_std_alloc() {
    int* ptr = (int*)alloc_malloc(&alloc_std, sizeof(int));
    assert(ptr != NULL);
    alloc_free(&alloc_std, ptr);
}

// test: alloc & free through custom_alloc_t
void test_custom_alloc() {
    custom_alloc_t counter = { .alloc_counter = 0 };
    alloc_t custom_alloc = {
            .calloc_fn = custom_calloc,
            .malloc_fn = custom_malloc,
            .realloc_fn = custom_realloc,
            .free_fn = custom_free,
            .extra = &counter
    };

    int prev_counter = counter.alloc_counter;
    int* ptr = (int*)alloc_malloc(&custom_alloc, sizeof(int));
    assert(ptr != NULL);
    assert(counter.alloc_counter == prev_counter + 1);

    prev_counter = counter.alloc_counter;
    alloc_free(&custom_alloc, ptr);
    assert(counter.alloc_counter == prev_counter - 1);
}

// test: calloc & free through custom_alloc_t
void test_custom_calloc() {
    custom_alloc_t counter = { .alloc_counter = 0 };
    alloc_t custom_alloc = {
            .calloc_fn = custom_calloc,
            .malloc_fn = custom_malloc,
            .realloc_fn = custom_realloc,
            .free_fn = custom_free,
            .extra = &counter
    };

    int prev_counter = counter.alloc_counter;
    int* arr = (int*)alloc_calloc(&custom_alloc, 10, sizeof(int));
    assert(arr != NULL);
    assert(counter.alloc_counter == prev_counter + 1);

    prev_counter = counter.alloc_counter;
    alloc_free(&custom_alloc, arr);
    assert(counter.alloc_counter == prev_counter - 1);
}

// test: realloc & free through custom_alloc_t
void test_custom_realloc() {
    custom_alloc_t counter = { .alloc_counter = 0 };
    alloc_t custom_alloc = {
            .calloc_fn = custom_calloc,
            .malloc_fn = custom_malloc,
            .realloc_fn = custom_realloc,
            .free_fn = custom_free,
            .extra = &counter
    };

    int prev_counter = counter.alloc_counter;
    int* arr = (int*)alloc_malloc(&custom_alloc, 5 * sizeof(int));
    assert(arr != NULL);
    assert(counter.alloc_counter == prev_counter + 1);

    prev_counter = counter.alloc_counter;
    int *new_arr = (int*)alloc_realloc(&custom_alloc, arr, 10 * sizeof(int));
    assert(new_arr != NULL);
    assert(counter.alloc_counter == prev_counter);

    prev_counter = counter.alloc_counter;
    alloc_free(&custom_alloc, new_arr);
    assert(counter.alloc_counter == prev_counter - 1);
}

int main() {
    test_std_alloc();
    printf("OK test_std_alloc\n");
    test_custom_alloc();
    printf("OK test_custom_alloc\n");
    test_custom_calloc();
    printf("OK test_custom_calloc\n");
    test_custom_realloc();
    printf("OK test_custom_realloc\n");

    printf("All alloc.h tests passed!\n");
    return 0;
}